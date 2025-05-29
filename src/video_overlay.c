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
#include "fonts/font_bf_default.h"
#include "logo/logo.h"
#include "canvas_char.h"

#define TIM2_TICK_MS        (1e6f / 170000000)

// OPAMP1 multiplexer constants
#define OPAMP_CONST_IO0     0x108000E1U  // Positive Input IO0 (e.g., PA1) -
#define OPAMP_CONST_IO1     0x108000E5U  // Positive Input IO1 (e.g., PA3) - video generator input
#define OPAMP_CONST_IO2     0x108000E9U  // Positive Input IO2 (e.g., PA7) - video input form camera
#define OPAMP_CONST_DAC     0x108000EDU  // Positive Input DAC1_OUT1 (internal DAC)

#define PEXELS_PER_LINE     (360)
#define BACK_PORCH_LEN      (26) // + DMA started with delay ~2.5us
#define OFFSET_Y            (16)
#define OFFSET_X            (BACK_PORCH_LEN)
#define LINE_BUF_SZ         (BACK_PORCH_LEN + OFFSET_X + PEXELS_PER_LINE+1)

#define DAC_BLACK           DAC12BIT_FROM_MV(450)
#define DAC_WHITE           DAC12BIT_FROM_MV(900)
#define DAC_GRAY            DAC12BIT_FROM_MV(650)
#define MAX_RENDER_LINE     (305) // for PAL

#define LOGO_OFFSET_X       (120)
#define LOGO_OFFSET_Y       (25)

typedef enum {
    PX_BLACK       = 0,
    PX_TRANSPARENT = 1,
    PX_WHITE       = 2,
    PX_GRAY        = 3
  } px_t;

static uint16_t dac_buff[2][LINE_BUF_SZ];   // DAC double buffer for draw pixel (12-bit CH1)  DMA HALF_WORLD/WORLD
static uint32_t opamp_buff[2][LINE_BUF_SZ]; // double buffer for OPAMP1 multiplexer (32-bit)  DMA WORLD/WORLD
CCMRAM_BSS static bool buf_idx = 0; // current buffer index for double buffering
CCMRAM_DATA static uint32_t video_source = OPAMP_CONST_IO2; // OPAMP_CONST_IO1 - video gen, OPAMP_CONST_IO2 - video input
extern volatile bool video_gen_enabled;
extern char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
extern uint8_t active_buffer;
CCMRAM_DATA bool show_logo = true;

EXEC_RAM static void init_buffers(bool curr_buff)
{
    for (uint32_t j = 0; j < LINE_BUF_SZ; j++) {
        dac_buff[curr_buff][j] = DAC_GRAY;
        opamp_buff[curr_buff][j] = video_source;
    }
}

static void show_version(void)
{
    char str[COLUMN_SIZE];
    sprintf(str, "FW: %s", FW_VERSION);
    canvas_char_write(8, 9, str, strlen(str));
    sprintf(str, "MCU: %s", MCU_TYPE);
    canvas_char_write(8, 10, str, strlen(str));
    canvas_char_draw_complete();
}

void video_overlay_init(void)
{
    DAC1_Init(); // DAC1_CH1 for video detection
    DAC3_Init(); // DAC3_CH1 for render line
    OPAMP1_Init(); // OPAMP1 as multiplexer for video source selection
    TIM1_Init(); // TIM1 for video line generation
    TIM2_Init(); // TIM2 detect HSYNC VSYNC video input
    TIM3_Init(); // TIM3 detect HSYNC VSYNC video generator
    TIM4_Init(); // TIM4 delay for start video generator
    TIM17_Init(); // TIM17 for video generator output (PWM ~31.8us)
    COMP3_Init(); // COMP3 for video sync detection
    COMP4_Init(); // COMP4 for video generator sync detection

    LL_OPAMP_Enable(OPAMP1);

    LL_COMP_Enable(COMP3);
    LL_COMP_Enable(COMP4);

    LL_TIM_EnableIT_CC1(TIM2);
    LL_TIM_EnableCounter(TIM2);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);

    LL_TIM_EnableIT_CC1(TIM3);
    LL_TIM_EnableCounter(TIM3);
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);

    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
    LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, DAC12BIT_FROM_MV(300));
    LL_DAC_TrigSWConversion(DAC1, LL_DAC_CHANNEL_1);

    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_1);
    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_1, DAC_BLACK);
    LL_DAC_TrigSWConversion(DAC3, LL_DAC_CHANNEL_1);

    LL_TIM_EnableIT_UPDATE(TIM4);
    LL_TIM_EnableCounter(TIM4);

    init_buffers(0);
    init_buffers(1);
    canvas_char_flush_map();

    show_version();
}

EXEC_RAM static void squash_canvas_raw_pixel_buff(char c, uint32_t glyph_row, uint32_t x_off)
{
    register const uint8_t * const glyph = &font_data[(uint8_t)c * FONT_STRIDE];
    register const uint32_t row_offset = glyph_row * BYTES_PER_ROW;
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

void render_overlay_logo_line(uint16_t line)
{
    if (line >= MAX_RENDER_LINE) return;

    // Logo line considering vertical offset
    if (line < LOGO_OFFSET_Y || (line - LOGO_OFFSET_Y) >= LOGO_HEIGHT) {
        // Line outside the logo area — do nothing, keep existing buffer contents
        return;
    }

    uint16_t logo_row = line - LOGO_OFFSET_Y;
    const uint8_t *logo_line_ptr = &logo_data[logo_row * LOGO_ROW_BYTES];

    // Buffer index starting from horizontal offset LOGO_OFFSET_X
    uint16_t buf_index = LOGO_OFFSET_X;

    for (uint16_t logo_col = 0; logo_col < LOGO_WIDTH; logo_col++) {
        if (buf_index >= LINE_BUF_SZ) break;

        uint32_t bit_pos = logo_col * 2;
        uint32_t byte_idx = bit_pos >> 3;
        uint32_t shift = 6 - (bit_pos & 7);

        px_t px = (px_t)((logo_line_ptr[byte_idx] >> shift) & 0x3);

        // If pixel is transparent — skip (keep existing buffer value)
        if (px == PX_TRANSPARENT) {
            buf_index++;
            continue;
        }

        uint16_t dac_val;
        uint32_t opa_val;

        switch (px) {
        case PX_BLACK:       dac_val = DAC_BLACK; opa_val = OPAMP_CONST_DAC; break;
        case PX_WHITE:       dac_val = DAC_WHITE; opa_val = OPAMP_CONST_DAC; break;
        case PX_GRAY:        dac_val = DAC_GRAY;  opa_val = OPAMP_CONST_DAC; break;
        case PX_TRANSPARENT:
        default:             dac_val = DAC_BLACK; opa_val = video_source;    break;
        }

        // Overlay — overwrite only if pixel is not transparent
        dac_buff[buf_idx][buf_index] = dac_val;
        opamp_buff[buf_idx][buf_index] = opa_val;

        buf_index++;
    }
}

EXEC_RAM static void render_line(uint16_t line)
{
    CCMRAM_BSS static uint32_t draw_line = 0;
    register uint32_t map_row = 0;
    register uint32_t glyph_row = 0;
    register uint32_t line_parity = 0;
    register uint32_t i = 0;
    register char c = 0;

    // Offset current draw line by Y_OFFSET
    if (line < OFFSET_Y) return;
    draw_line = line - OFFSET_Y;

    // Calculate which character row and which row in the glyph
    map_row    = draw_line / FONT_HEIGHT;  // 0..ROW_SIZE-1
    glyph_row  = draw_line % FONT_HEIGHT;  // 0..FONT_HEIGHT-1

    line_parity = draw_line & 1;

    for (i = 0; i < OFFSET_X; i++) {
        opamp_buff[line_parity][i] = video_source;
    }

    if (map_row >= ROW_SIZE) {
        // Out of screen — just transparent
        for (i = OFFSET_X; i < LINE_BUF_SZ; i++) {
            dac_buff[line_parity][i] = DAC_BLACK;
            opamp_buff[line_parity][i] = video_source;
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

EXEC_RAM static void push_line_to_dma(uint16_t line)
{
    buf_idx = (line & 1);
    // Stop TIM1 and both DMA channels
    LL_TIM_DisableCounter(TIM1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);

    // Configure length and addresses
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)&opamp_buff[!buf_idx][0]); // ! send previous buffer
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, LINE_BUF_SZ);
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)&dac_buff[!buf_idx][0]); // ! send previous buffer
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, LINE_BUF_SZ);

    // Enable DMA request for DAC
    LL_DAC_EnableDMAReq(DAC3, LL_DAC_CHANNEL_1);

    // Enable both DMA channels
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2); // first start DAC channel
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

    LL_TIM_EnableDMAReq_UPDATE(TIM1);

    LL_TIM_EnableCounter(TIM1);

    render_line(line);
    if (show_logo) {
        render_overlay_logo_line(line);
    }

}

EXEC_RAM static inline void pars_video_signal(uint32_t tim_tick)
{
    CCMRAM_BSS static uint16_t video_line = 0;

    register float time_ns = (float)tim_tick * TIM2_TICK_MS;
    if(time_ns > 59.f && time_ns < 65.f) {
        video_line++;
        if (video_line <= MAX_RENDER_LINE) {
            push_line_to_dma(video_line);
        }
    } else if (time_ns > 29.0f && time_ns < 33.0f) {
        video_line = 0;
    } else {
        // Do nothing, wait for next sync
    }
}

// Called from COMP3 & TIM2 event (video input)
EXEC_RAM void TIM2_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_CC1(TIM2)) {
        TIM4->CNT = 0;
        video_source = OPAMP_CONST_IO2;
        OPAMP1->CSR = video_source;
        if (video_gen_enabled == true) {
            video_gen_stop();
            LL_TIM_DisableCounter(TIM1);
            LL_TIM_SetETRSource(TIM1, LL_TIM_TIM1_ETRSOURCE_COMP3);
        } else {
            pars_video_signal(TIM2->CCR1);
        }
        LL_TIM_ClearFlag_CC1(TIM2);
    }
}

// Called form COMP4 & TIM3 event (video gen)
EXEC_RAM void TIM3_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_CC1(TIM3)) {
        TIM4->CNT = 0;
        video_source = OPAMP_CONST_IO1;
        OPAMP1->CSR = video_source;
        if (video_gen_enabled == false) {
            video_gen_start();
            LL_TIM_DisableCounter(TIM1);
            LL_TIM_SetETRSource(TIM1, LL_TIM_TIM1_ETRSOURCE_COMP4);
        } else {
            pars_video_signal(TIM3->CCR1);
        }
        LL_TIM_ClearFlag_CC1(TIM3);
    }
}

// Called when video from the camera is not detected
EXEC_RAM void TIM4_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM4)) {
        if (video_gen_enabled == false) {
            video_gen_start();
            LL_TIM_DisableCounter(TIM1);
            LL_TIM_SetETRSource(TIM1, LL_TIM_TIM1_ETRSOURCE_COMP4);
        }
        LL_TIM_ClearFlag_UPDATE(TIM4);
    }
}
