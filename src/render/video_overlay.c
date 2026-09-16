/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "video_overlay.h"
#include "system.h"
#include "main.h"
#include "video_gen.h"
#include "logo/logo.h"
#include "canvas_char.h"
#include "video_graphics.h"
#if defined(LOW_RAM)
#include "fonts/font_bf_default.h"
#endif

#define TIM2_TICK_MS        (1e6f / 170000000)

// OPAMP1 multiplexer constants
#define OPAMP_CONST_IO0     0x108000E1U  // Positive Input IO0 (e.g., PA1)
#define OPAMP_CONST_IO1     0x108000E5U  // Positive Input IO1 (PA3) - camera 1
#define OPAMP_CONST_IO2     0x108000E9U  // Positive Input IO2 (PA7) - camera 2
#define OPAMP_CONST_DAC     0x108000EDU  // Positive Input DAC3_CH1 (internal DAC)

#define PEXELS_PER_LINE     (360)
#define OFFSET_Y            (16)
#define OFFSET_X            (0) // TIM15 supplies the back-porch delay
#define LINE_BUF_SZ         (PEXELS_PER_LINE + 1)

#define DAC_BLACK           DAC12BIT_FROM_MV(450)
#define DAC_WHITE           DAC12BIT_FROM_MV(900)
#define DAC_GRAY            DAC12BIT_FROM_MV(650)
#define MAX_RENDER_LINE     (305) // for PAL

#define LOGO_OFFSET_X       (75)
#define LOGO_OFFSET_Y       (50)

typedef struct {
    uint32_t comp_input;
    uint32_t opamp_input;
} video_input_config_t;

// COMP2 IO2=PA3 and IO1=PA7; OPAMP1 uses IO1=PA3 and IO2=PA7.
static const video_input_config_t video_inputs[2] = {
    { LL_COMP_INPUT_PLUS_IO2, OPAMP_CONST_IO1 },
    { LL_COMP_INPUT_PLUS_IO1, OPAMP_CONST_IO2 }
};

static uint16_t dac_buff[2][LINE_BUF_SZ];   // DAC double buffer for draw pixel (12-bit CH1)  DMA HALF_WORLD/WORLD
static uint32_t opamp_buff[2][LINE_BUF_SZ]; // double buffer for OPAMP1 multiplexer (32-bit)  DMA WORLD/WORLD
// Packed RAM copy avoids flash accesses in the time-critical logo renderer.
static uint8_t logo_cache[LOGO_HEIGHT][LOGO_ROW_BYTES];
static uint16_t logo_first_pixel[LOGO_HEIGHT];
static uint16_t logo_end_pixel[LOGO_HEIGHT];
static const uint16_t logo_levels[4] __attribute__((section(".ccmram.rodata"))) = { DAC_BLACK, DAC_BLACK, DAC_WHITE, DAC_GRAY };
CCMRAM_BSS static bool buf_idx = 0; // current buffer index for double buffering
CCMRAM_DATA static uint32_t video_source = OPAMP_CONST_IO2;
CCMRAM_DATA static video_input_t active_video_input = VIDEO_INPUT_2;
extern char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
extern uint8_t active_buffer;
CCMRAM_DATA bool show_logo = true;
CCMRAM_BSS bool new_field = false;
CCMRAM_BSS static uint16_t video_line = 0;
CCMRAM_BSS static bool field_synced = false;
CCMRAM_BSS static bool discard_capture = false;
CCMRAM_BSS static uint8_t half_lines = 0;
static uint16_t sync_threshold_mv[2] = { VIDEO_SYNC_START_MV, VIDEO_SYNC_START_MV };
static uint32_t last_scan_ms;
static volatile video_diagnostics_t video_diagnostics;
static uint16_t sync_low_raw;
static bool sync_low_valid;
static bool sync_sample_pending;

EXEC_RAM static void update_sync_reference(uint16_t low, uint16_t black)
{
    // Reject stale/invalid samples, then move gradually towards the centre
    // of the sync pulse amplitude instead of staying at the first scan hit.
    if (black <= low) return;
    uint16_t amplitude = black - low;
    if (amplitude < DAC12BIT_FROM_MV(100) || amplitude > DAC12BIT_FROM_MV(600)) return;
    uint16_t midpoint = DAC12BIT_TO_MV(low + amplitude / 2U);
    if (midpoint < VIDEO_SYNC_MIN_MV || midpoint > VIDEO_SYNC_MAX_MV) return;
    uint16_t previous = sync_threshold_mv[active_video_input];
    uint16_t next = (3U * previous + midpoint + 2U) / 4U;
    sync_threshold_mv[active_video_input] = next;
    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, DAC12BIT_FROM_MV(next));
    video_diagnostics.threshold_mv = next;
}

#if defined(HIGH_RAM)
extern uint8_t active_video_buffer;
extern uint8_t video_frame_buffer[2][VIDEO_HEIGHT][VIDEO_BYTES_PER_LINE];
#endif

EXEC_RAM static void init_buffers()
{
    for (uint32_t j = 0; j < LINE_BUF_SZ; j++) {
        dac_buff[0][j] = DAC_GRAY;
        opamp_buff[0][j] = video_source;
        dac_buff[1][j] = DAC_GRAY;
        opamp_buff[1][j] = video_source;
    }
}

static void init_logo_cache(void)
{
    // Read the dedicated .logo region rather than letting the compiler create
    // another constant copy in .rodata (the logo can be flashed separately).
    const volatile uint8_t *flash_logo = logo_data;
    for (uint32_t i = 0; i < sizeof(logo_cache); i++) ((uint8_t *)logo_cache)[i] = flash_logo[i];
    for (uint32_t row = 0; row < LOGO_HEIGHT; row++) {
        uint16_t first = LOGO_WIDTH;
        uint16_t end = 0;
        for (uint32_t col = 0; col < LOGO_WIDTH; col++) {
            uint8_t pixel = (logo_cache[row][col / 4U] >> (6U - 2U * (col % 4U))) & 3U;
            if (pixel != PX_TRANSPARENT) {
                if (first == LOGO_WIDTH) first = col;
                end = col + 1U;
            }
        }
        logo_first_pixel[row] = first;
        logo_end_pixel[row] = end;
    }
}

static void show_version(void)
{
   char str[COLUMN_SIZE];
    sprintf(str, "FORKED BY: THOMASJKLV");
    canvas_char_write(4, 9, str, strlen(str));
    sprintf(str, "MCU: %s", MCU_TYPE);
    canvas_char_write(8, 10, str, strlen(str));
    canvas_char_draw_complete();
}

EXEC_RAM static void stop_pixel_output(void)
{
    LL_TIM_DisableDMAReq_UPDATE(TIM1);
    LL_TIM_DisableDMAReq_CC2(TIM1);
    LL_TIM_DisableCounter(TIM1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
    // Read flags before clearing them: arming DMA does not prove pixels ran.
    if (LL_DMA_IsActiveFlag_TC1(DMA1)) video_diagnostics.completed_lines++;
    if (LL_DMA_IsActiveFlag_TE1(DMA1) || LL_DMA_IsActiveFlag_TE2(DMA1)) {
        video_diagnostics.dma_errors++;
    }
    LL_DMA_ClearFlag_GI1(DMA1);
    LL_DMA_ClearFlag_GI2(DMA1);
}

void set_video_input(video_input_t input)
{
    if (input != VIDEO_INPUT_1 && input != VIDEO_INPUT_2) {
        return;
    }

    // Protect the renderer state from TIM2 while changing both signal paths.
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t capture_enabled = LL_TIM_CC_IsEnabledChannel(TIM2, LL_TIM_CHANNEL_CH2);
    LL_TIM_CC_DisableChannel(TIM2, LL_TIM_CHANNEL_CH2);
    // Quiesce hardware triggers as well as CPU interrupts during a switch.
    LL_TIM_SetSlaveMode(TIM15, LL_TIM_SLAVEMODE_DISABLED);
    LL_TIM_DisableCounter(TIM15);
    LL_TIM_SetSlaveMode(TIM1, LL_TIM_SLAVEMODE_DISABLED);
    stop_pixel_output();

    active_video_input = input;

    // Select the same physical pin for sync detection and video passthrough.
    LL_COMP_SetInputPlus(COMP2, video_inputs[input].comp_input);
    video_source = video_inputs[input].opamp_input;
    OPAMP1->CSR = video_source;

    // No queued DMA word may select the previous camera after this point.
    init_buffers();
    buf_idx = 0;
    video_line = 0;
    field_synced = false;
    half_lines = 0;
    sync_low_valid = false;
    sync_sample_pending = false;
    LL_TIM_OC_SetCompareCH1(TIM2, VIDEO_BLACK_SAMPLE_TICKS);
    LL_ADC_ClearFlag_JEOC(ADC1);
    discard_capture = true;
    new_field = false;
    last_scan_ms = HAL_GetTick();
    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, DAC12BIT_FROM_MV(sync_threshold_mv[input]));
    video_diagnostics.threshold_mv = sync_threshold_mv[input];
    video_diagnostics.input = input;
    video_diagnostics.locked = false;

    // Start timing acquisition cleanly after switching cameras.
    LL_TIM_SetCounter(TIM2, 0);
    LL_TIM_ClearFlag_CC2(TIM2);
    LL_TIM_ClearFlag_CC2OVR(TIM2);
    LL_TIM_ClearFlag_CC3(TIM2);
    NVIC_ClearPendingIRQ(TIM2_IRQn);
    LL_TIM_SetSlaveMode(TIM1, LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER);
    LL_TIM_SetSlaveMode(TIM15, LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER);
    if (capture_enabled) {
        LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);
    }
    __set_PRIMASK(primask);
}

video_input_t get_video_input(void)
{
    return active_video_input;
}

void video_get_diagnostics(video_diagnostics_t *result)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    *result = video_diagnostics;
    __set_PRIMASK(primask);
}

void video_sync_loop(void)
{
    uint32_t now = HAL_GetTick();
    if (now - last_scan_ms < VIDEO_SYNC_SCAN_MS) return;
    // A capture IRQ can publish a timestamp newer than 'now'. Such a field
    // is fresh, not an unsigned wraparound indicating long-lost sync.
    if (video_diagnostics.locked && (int32_t)(now - video_diagnostics.last_field_ms) < (int32_t)VIDEO_SYNC_LOST_MS) return;

    // Use the same 25..800 mV search range as Telekatz, independently per input.
    // set_video_input() safely stops the hardware before changing the reference.
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    now = HAL_GetTick();
    // An IRQ may have acquired sync after the check above.
    if (!(video_diagnostics.locked && (int32_t)(now - video_diagnostics.last_field_ms) < (int32_t)VIDEO_SYNC_LOST_MS)) {
        uint16_t next = sync_threshold_mv[active_video_input] + VIDEO_SYNC_STEP_MV;
        sync_threshold_mv[active_video_input] = next > VIDEO_SYNC_MAX_MV ? VIDEO_SYNC_MIN_MV : next;
        set_video_input(active_video_input);
    }
    __set_PRIMASK(primask);
}

void video_overlay_init(void)
{
    init_buffers();
    init_logo_cache(); // Prepare before enabling video interrupts.
    canvas_char_flush_map();

#if defined(HIGH_RAM)
    video_graphics_init();
#endif

    DAC3_Init(); // CH1 renders pixels; CH2 is the COMP2 sync threshold
    OPAMP1_Init(); // OPAMP1 as multiplexer for video source selection
    TIM1_Init(); // TIM1 for video line generation
    TIM2_Init(); // TIM2 detect HSYNC VSYNC video input
    TIM15_Init(); // Fixed active-video start delay, as in Telekatz
    COMP2_Init(); // COMP2 detects sync on the selected camera input

    set_video_input(VIDEO_INPUT_2); // Match Telekatz generic: start on PA7

    LL_OPAMP_Enable(OPAMP1);

    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_2);
    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, DAC12BIT_FROM_MV(sync_threshold_mv[active_video_input]));
    LL_DAC_TrigSWConversion(DAC3, LL_DAC_CHANNEL_2);

    LL_COMP_Enable(COMP2);

    LL_TIM_EnableIT_CC2(TIM2);
    LL_TIM_EnableIT_CC3(TIM2);
    LL_TIM_EnableCounter(TIM2);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);

    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_1);
    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_1, DAC_BLACK);
    LL_DAC_TrigSWConversion(DAC3, LL_DAC_CHANNEL_1);

    show_version();
}

#if defined(LOW_RAM)
EXEC_RAM static void squash_canvas_raw_pixel_buff(char c, uint32_t glyph_row, uint32_t x_off)
{
    register const uint8_t * const glyph = &font_data[(uint8_t)c * FONT_STRIDE];
    register const uint32_t row_offset = glyph_row * BYTES_PER_ROW;
    // The CPU-owned line is already transparent. Skip blank glyph rows
    // instead of decoding/writing 360 background pixels under the logo.
    uint8_t opaque = 0;
    for (uint32_t i = 0; i < BYTES_PER_ROW; i++) opaque |= glyph[row_offset + i] ^ 0x55U;
    if (opaque == 0) return;
    register uint16_t dac_val;
    register uint32_t opa_val;

    for(register uint32_t col = 0; col < FONT_WIDTH; col++) {
        register uint32_t bitpos     = col * FONT_BPP;
        register uint32_t byte_index = row_offset + (bitpos >> 3);
        register uint32_t bit_offset = bitpos & 0x7;

        register uint8_t raw_byte = glyph[byte_index];
        register uint8_t pixel = (raw_byte >> (6 - bit_offset)) & 0x03;

        switch (pixel) {
        case PX_BLACK: dac_val = DAC_BLACK; opa_val = OPAMP_CONST_DAC; break;
        case PX_WHITE: dac_val = DAC_WHITE; opa_val = OPAMP_CONST_DAC; break;
        case PX_GRAY:  dac_val = DAC_GRAY;  opa_val = OPAMP_CONST_DAC; break;
        default:       dac_val = DAC_BLACK; opa_val = video_source;    break;
        }

        dac_buff[buf_idx][x_off + col] = dac_val;
        opamp_buff[buf_idx][x_off + col] = opa_val;
    }
    opamp_buff[buf_idx][x_off + FONT_WIDTH] = video_source;
}
#endif

EXEC_RAM static void render_overlay_logo_line(uint16_t line)
{
    if (line > MAX_RENDER_LINE || line < LOGO_OFFSET_Y ||
        line - LOGO_OFFSET_Y >= LOGO_HEIGHT || LOGO_OFFSET_X >= PEXELS_PER_LINE) return;

    uint32_t row = line - LOGO_OFFSET_Y;
    uint32_t end = logo_end_pixel[row];
    if (end > PEXELS_PER_LINE - LOGO_OFFSET_X) end = PEXELS_PER_LINE - LOGO_OFFSET_X;
    uint16_t *dac = dac_buff[buf_idx];
    uint32_t *opamp = opamp_buff[buf_idx];
    const uint8_t *packed = logo_cache[row];
    for (uint32_t col = logo_first_pixel[row]; col < end; col++) {
        uint8_t pixel = (packed[col / 4U] >> (6U - 2U * (col % 4U))) & 3U;
        if (pixel != PX_TRANSPARENT) {
            dac[LOGO_OFFSET_X + col] = logo_levels[pixel];
            opamp[LOGO_OFFSET_X + col] = OPAMP_CONST_DAC;
        }
    }
    // Preserve the final passthrough word, even when clipping a wide logo.
}

#if defined(LOW_RAM)
EXEC_RAM static void render_line(uint16_t line)
{
    CCMRAM_BSS static uint32_t draw_line = 0;
    register uint32_t map_row = 0;
    register uint32_t glyph_row = 0;
    register uint32_t i = 0;
    register char c = 0;

    // Offset current draw line by Y_OFFSET
    if (line < OFFSET_Y) return;
    draw_line = line - OFFSET_Y;

    // Calculate which character row and which row in the glyph
    map_row    = draw_line / FONT_HEIGHT;  // 0..ROW_SIZE-1
    glyph_row  = draw_line % FONT_HEIGHT;  // 0..FONT_HEIGHT-1

    if (map_row >= ROW_SIZE) {
        // Out of screen — just transparent
        for (i = OFFSET_X; i < LINE_BUF_SZ; i++) {
            dac_buff[buf_idx][i] = DAC_BLACK;
            opamp_buff[buf_idx][i] = video_source;
        }
        return;
    }

    // Render each character of the map
    for (i = 0; i < COLUMN_SIZE; i++) {
        c = canvas_char_map[!active_buffer][map_row][i]; // draw previous char map buffer
        squash_canvas_raw_pixel_buff(c, glyph_row, OFFSET_X + i * FONT_WIDTH);
    }

    opamp_buff[buf_idx][LINE_BUF_SZ-1] = video_source;
}
#endif

#if defined(HIGH_RAM)
EXEC_RAM void render_video_line(uint16_t line)
{
    CCMRAM_BSS static uint32_t draw_line = 0;

    for (register int i = 0; i < OFFSET_X; i++) {
        opamp_buff[buf_idx][i] = video_source;
    }

    if (line < OFFSET_Y || (uint32_t)(line - OFFSET_Y) >= VIDEO_HEIGHT) {
        for (register int i = OFFSET_X; i < LINE_BUF_SZ; i++) {
            opamp_buff[buf_idx][i] = video_source;
        }
        return;
    }
    draw_line = line - OFFSET_Y;

    uint8_t *line_ptr = video_frame_buffer[!active_video_buffer][draw_line];

    uint32_t buf_idx_local = OFFSET_X;
    for (uint32_t i = 0; i < VIDEO_BYTES_PER_LINE; i++) {
        uint8_t byte = line_ptr[i];
        for (int shift = 6; shift >= 0; shift -= 2) {
            if (buf_idx_local >= LINE_BUF_SZ) break;

            uint8_t pixel = (byte >> shift) & 0x3;

            uint16_t dac_val;
            uint32_t opa_val;

            switch (pixel) {
            case PX_BLACK:       dac_val = DAC_BLACK; opa_val = OPAMP_CONST_DAC; break;
            case PX_WHITE:       dac_val = DAC_WHITE; opa_val = OPAMP_CONST_DAC; break;
            case PX_GRAY:        dac_val = DAC_GRAY;  opa_val = OPAMP_CONST_DAC; break;
            case PX_TRANSPARENT:
            default:             dac_val = DAC_BLACK; opa_val = video_source; break;
            }

            dac_buff[buf_idx][buf_idx_local] = dac_val;
            opamp_buff[buf_idx][buf_idx_local] = opa_val;

            buf_idx_local++;
        }
    }
    opamp_buff[buf_idx][LINE_BUF_SZ-1] = video_source;
}
#endif

EXEC_RAM static void push_line_to_dma(uint16_t line)
{
    video_diagnostics.armed_lines++;
    buf_idx = (line & 1);
    stop_pixel_output();
    LL_TIM_SetCounter(TIM1, 0);

    // Configure length and addresses
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)&opamp_buff[!buf_idx][0]); // ! send previous buffer
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, LINE_BUF_SZ);
    // A DAC trigger latches DHR before TIM1_UP DMA loads the next value.
    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_1, dac_buff[!buf_idx][0]);
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)&dac_buff[!buf_idx][1]);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, LINE_BUF_SZ - 1);

    // Enable both DMA channels
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2); // first start DAC channel
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

    /* DAC first on update, OPAMP selection 16 TIM1 clocks later on CH2. */
    LL_TIM_EnableDMAReq_UPDATE(TIM1);
    LL_TIM_EnableDMAReq_CC2(TIM1);

    // TIM15 starts TIM1 at a fixed point after sync; do not start in software.

    // Clear only the CPU-owned buffer, including porch and trailing pixels.
    for (uint32_t i = 0; i < LINE_BUF_SZ; i++) {
        opamp_buff[buf_idx][i] = video_source;
        dac_buff[buf_idx][i] = DAC_BLACK;
    }

#if defined(HIGH_RAM)
    render_video_line(line); // video frame buffer
#else
    render_line(line); // char canvas map
#endif
    if (show_logo) {
        render_overlay_logo_line(line);
    }

}

EXEC_RAM static inline void pars_video_signal(uint32_t tim_tick)
{
    if (discard_capture) {
        discard_capture = false;
        return;
    }

    register float time_ns = (float)tim_tick * TIM2_TICK_MS;
    if(time_ns > 59.f && time_ns < 67.5f) {
        video_diagnostics.full_lines++;
        // A burst of equalizing/vertical half-lines marks a new field.
        // Reject isolated half-length noise pulses as a field boundary.
        if (half_lines >= 4) {
            video_line = 0;
            field_synced = true;
            video_diagnostics.fields++;
            video_diagnostics.last_field_ms = HAL_GetTick();
            video_diagnostics.locked = true;
        }
        half_lines = 0;
        sync_low_valid = false;
        sync_sample_pending = false;
        LL_TIM_OC_SetCompareCH1(TIM2, VIDEO_BLACK_SAMPLE_TICKS);
        if (!field_synced) return;
        if (video_line > MAX_RENDER_LINE) return;
        video_line++;
        if (video_line <= MAX_RENDER_LINE) {
            push_line_to_dma(video_line);
        }
        if (new_field == false) {
            new_field = true;
        }
    } else if (time_ns > 29.0f && time_ns < 35.5f) {
        if (half_lines < 255) half_lines++;
        if (half_lines == 5 && sync_sample_pending) {
            sync_low_valid = LL_ADC_IsActiveFlag_JEOC(ADC1);
            if (sync_low_valid) sync_low_raw = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1);
            LL_ADC_ClearFlag_JEOC(ADC1);
            LL_TIM_OC_SetCompareCH1(TIM2, VIDEO_BLACK_SAMPLE_TICKS);
            sync_sample_pending = false;
        }
        if (half_lines == 11 && sync_low_valid) {
            if (LL_ADC_IsActiveFlag_JEOC(ADC1)) {
                update_sync_reference(sync_low_raw, LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1));
            }
            sync_low_valid = false;
            LL_ADC_ClearFlag_JEOC(ADC1);
        }
        if (new_field == true) {
            new_field = false;
        }
    } else if (time_ns > 56.0f && time_ns < 58.0f) {
        if (half_lines >= 4) {
            // Telekatz's broad-VSYNC transition: measure the low plateau
            // on this pulse, then black after the trailing equalizing pulses.
            half_lines = 4;
            sync_low_valid = false;
            sync_sample_pending = true;
            LL_ADC_ClearFlag_JEOC(ADC1);
            LL_TIM_OC_SetCompareCH1(TIM2, VIDEO_SYNC_SAMPLE_TICKS);
        }
    } else if (time_ns > 6.5f && time_ns < 7.5f) {
        // Rising-edge intervals at the transitions into/out of broad VSYNC.
        // Preserve the field candidate, as in Telekatz's sync parser.
    } else {
        half_lines = 0;
        sync_low_valid = false;
        sync_sample_pending = false;
        LL_TIM_OC_SetCompareCH1(TIM2, VIDEO_BLACK_SAMPLE_TICKS);
    }
}

// Called from the selected camera through COMP2 and TIM2 channel 2.
EXEC_RAM void TIM2_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_CC3(TIM2)) {
        LL_TIM_ClearFlag_CC3(TIM2);
        stop_pixel_output();
        OPAMP1->CSR = video_source;
    }
    if (LL_TIM_IsActiveFlag_CC2(TIM2)) {
        uint32_t ticks = TIM2->CCR2;
        video_diagnostics.captures++;
        video_diagnostics.last_capture_ticks = ticks;
        LL_TIM_ClearFlag_CC2(TIM2);
        stop_pixel_output();
        OPAMP1->CSR = video_source;
        pars_video_signal(ticks);
    }
}

// The external fallback generator is disabled because PA3 is camera input 1.
EXEC_RAM void TIM3_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_CC1(TIM3)) {
        LL_TIM_ClearFlag_CC1(TIM3);
    }
}

EXEC_RAM void TIM4_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM4)) {
        LL_TIM_ClearFlag_UPDATE(TIM4);
    }
}
