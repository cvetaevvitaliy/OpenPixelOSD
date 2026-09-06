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
#include "settings.h"
#include "video_gen.h"
#include "canvas_char.h"
#include "video_graphics.h"
#include "led.h"
#include "video_color.h"

#if !defined(USE_GRAPHICS)
#include "fonts/font_betaflight.h"
#include "logo/logo.h"
#endif

#define TIM2_TICK_MS        (1e6f / 170000000)

// OPAMP1 multiplexer constants
#define OPAMP_CONST_IO0     0x108000E1U  // Positive Input IO0 (e.g., PA1) -
#define OPAMP_CONST_IO1     ((VIDEO_TOTAL_GAIN / VIDEO2_INPUT_GAIN) == 1 ? 0x108000E5U : 0x108000C5U)  // Positive Input IO1 (e.g., PA3) - video generator input
#define OPAMP_CONST_IO2     ((VIDEO_TOTAL_GAIN / VIDEO1_INPUT_GAIN) == 1 ? 0x108000E9U : 0x108000C9U)  // Positive Input IO2 (e.g., PA7) - video input form camera

#define OPAMP_CONST_DAC     0x108000EDU  // Positive Input DAC1_OUT1 (internal DAC) Follower mode
#define OPAMP_CONST_DAC_MOD 0x108200CDU  // Positive Input DAC1_OUT1 (internal DAC) non inverting gain =2 with VINM0 pin for input or bias

#define OFFSET_SYNC         300
#define OFFSET_COLOR_LUM    100

#define DAC_BLACK           DAC12BIT_FROM_MV(550)

#define MAX_RENDER_LINE_PAL   (303)
#define MAX_RENDER_LINE_NTSC  (250)

#define LOGO_OFFSET_X       (90)
#define LOGO_OFFSET_Y       (25)

#define BITMASK(n)          ((1U << (n)) - 1U)


uint32_t opa_vals[16] = {OPAMP_CONST_DAC};
uint16_t video_levels[16] = {0};

uint16_t sync_levels[2] = {0};

uint8_t frame_counter = 0;
syncMode_t syncMode = OFF;
videoMode_t videoMode = MODE_UNKNOWN;
syncState_t syncState = SYNC_STATE_SEARCH;
uint16_t sync_voltage = SYNC_START_MV;
uint16_t sync_voltage_black = SYNC_START_MV;
uint16_t sync_voltage_low = 0;
osdState_e osdState = OSD_INIT;
static uint8_t sync_lost = SYNC_LOST_FRAMES_THRESHOLD;
uint16_t minRenderLine = 14;
uint16_t maxRenderLine = 0;

const videoInput_t videoInputs[2] = { {LL_COMP_INPUT_PLUS_IO1, OPAMP_CONST_IO2, VIDEO1_INPUT_GAIN},
                                      {LL_COMP_INPUT_PLUS_IO2, OPAMP_CONST_IO1, VIDEO2_INPUT_GAIN}};
uint8_t activeVideoInput;

uint16_t dac_buff[1 + DMA_DOUBLE_BUFFER][LINE_BUF_SZ];   // DAC double buffer for draw pixel (12-bit CH1)  DMA HALF_WORD/WORD
uint32_t opamp_buff[1 + DMA_DOUBLE_BUFFER][LINE_BUF_SZ]; // double buffer for OPAMP1 multiplexer (32-bit)  DMA WORD/WORD


CCMRAM_BSS static bool buf_idx = 0; // current buffer index for double buffering
CCMRAM_DATA static uint32_t video_source;
extern uint8_t active_buffer;
CCMRAM_DATA bool show_logo = true;
CCMRAM_DATA bool show_test_pattern = false;
CCMRAM_BSS bool new_field = false;

#ifdef TRIGGER_LINE
uint16_t triggerLine = TRIGGER_LINE;
#endif

EXEC_RAM static void set_black_level(uint32_t new_level)
{
  if (new_level > DAC12BIT_FROM_MV(OFFSET_SYNC))
    sync_levels[0] = (new_level - DAC12BIT_FROM_MV(OFFSET_SYNC)) * VIDEO_TOTAL_GAIN;
  else
    sync_levels[0] = 0;
  sync_levels[1] = new_level * VIDEO_TOTAL_GAIN;
  
  for (uint8_t x = 0; x<16; x++) {
    if (colorMap[colorMapIdx][x].phase > 0 && !video_gen_enabled) 
      video_levels[x] = new_level + DAC12BIT_FROM_MV(colorMap[colorMapIdx][x].luminance + OFFSET_COLOR_LUM);
    else
      video_levels[x] = (new_level + DAC12BIT_FROM_MV(colorMap[colorMapIdx][x].luminance)) * VIDEO_TOTAL_GAIN;
  } 

}

static void init_buffers()
{
    for(uint8_t b = 0; b <= DMA_DOUBLE_BUFFER; b++) {
      for (uint32_t j = 0; j < LINE_BUF_SZ; j++) {
          dac_buff[b][j] = DAC_BLACK;
          opamp_buff[b][j] = video_source;
          IF_USE_COLOR(phase_buff[b][j] = 0);
      }
    }
}

static void set_video_source(uint32_t source)
{
  video_source = source;
  for (uint8_t x = 0; x<16; x++) {
    if (colorMap[colorMapIdx][x].phase < 0) {
      opa_vals[x] = video_source;
    } else if (colorMap[colorMapIdx][x].phase > 0 && !video_gen_enabled) {
      opa_vals[x] = OPAMP_CONST_DAC_MOD;
    } else {
      opa_vals[x] = OPAMP_CONST_DAC;
    }
  }
  OPAMP1->CSR = source;
}

void set_video_input(uint8_t input)
{
  if(input)
    activeVideoInput = 1;
  else
    activeVideoInput = 0;

  sync_lost = 1;

  LL_COMP_SetInputPlus(COMP2, videoInputs[activeVideoInput].compInput);
}

void set_video_mode(videoMode_t mode)
{
  videoMode = mode;

  if(mode == MODE_NTSC) {
    TRACE_INFO_WP("videoMode NTSC\n");
    minRenderLine = 14;
    maxRenderLine = MAX_RENDER_LINE_NTSC;
  } else {
    TRACE_INFO_WP("videoMode PAL\n");
    #if defined(USE_HD)
    minRenderLine = MAX(14,158 - (ROW_SIZE * 9U) / 2);
    #else
    minRenderLine = MAX(14,158 - (ROW_SIZE * 9U));
    #endif
    maxRenderLine = MAX_RENDER_LINE_PAL;
  }

}

static void show_version(void)
{
    char str[COLUMN_SIZE];

    sprintf(str, "MCU: %s", MCU_TYPE);
    uint8_t col = (COLUMN_SIZE - strlen(str)) / 2;
    canvas_char_write(col, ROW_SIZE - 6, str, strlen(str), 0);
    sprintf(str, "FW: %s", FW_VERSION);
    canvas_char_write(col, ROW_SIZE - 7, str, strlen(str), 0);
    canvas_char_draw_complete();
}

void video_overlay_init(void)
{
    init_buffers();
    canvas_char_flush_map();

#if defined(USE_GRAPHICS)
    video_graphics_init();
#endif

#ifdef USE_COLOR
    video_color_init();
#endif

    DAC1_Init(); // DAC1_CH1 for video detection
    DAC3_Init(); // DAC3_CH1 for render line
    OPAMP1_Init(); // OPAMP1 as multiplexer for video source selection
    TIM1_Init(); // TIM1 for video line generation
    TIM2_Init(); // TIM2 detect HSYNC VSYNC video input
    
    TIM15_Init(); // TIM15 delay for video line generation start
    TIM17_Init(); // TIM17 for video generator
    COMP2_Init(); // COMP2 for video sync detection
    
    LL_OPAMP_Enable(OPAMP1);

    LL_COMP_Enable(COMP2);
    LL_COMP_Enable(COMP3);

    LL_TIM_EnableIT_CC2(TIM2);
    LL_TIM_EnableIT_CC3(TIM2);
    LL_TIM_EnableCounter(TIM2);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH2);

    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_2);
    LL_DAC_Enable(DAC3, LL_DAC_CHANNEL_1);

    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);

    LL_TIM_EnableIT_UPDATE(TIM17);

    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, (DAC12BIT_FROM_MV(SYNC_START_MV)) * videoInputs[activeVideoInput].gain);

    show_version();
    video_gen_enabled = true;
    video_gen_stop();
#if(VIDEO1_INPUT_ENABLED == false)
    set_video_input(1);
#elif(VIDEO2_INPUT_ENABLED == false)
    set_video_input(0);
#else
    set_video_input(settings.activeVideoInput);
#endif
    
    set_video_mode(MODE_PAL);
    video_gen_start();
    set_black_level(DAC_BLACK);
    set_video_source(OPAMP_CONST_DAC);

    if (settings.displayportEnabled) {
      setSyncMode(AUTOMATIC);
      osdState = OSD_MSP;
    } else {
      setSyncMode(OFF);
      osdState = OSD_OFF;
    }
}

void scan_sync_voltage() {
    sync_voltage += SYNC_SCAN_INC_MV;
    
    if(sync_voltage > SYNC_SCAN_MAX_MV) {
      sync_voltage = SYNC_SCAN_MIN_MV;
    } 

    LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, DAC12BIT_FROM_MV(sync_voltage) * videoInputs[activeVideoInput].gain);
}

#if defined(USE_GRAPHICS)

EXEC_RAM void render_video_line(uint16_t line)
{
    uint16_t* luminance;
    uint32_t* opamp;

    luminance = &video_levels[0];
    opamp = &opa_vals[0];

    #ifdef USE_COLOR
    uint32_t* phase;
    phase = &phase_val[palPhase][0];
    phase_buff[buf_idx][0] = phase[2];
    #endif

    if ((line < minRenderLine) || line >= minRenderLine + VIDEO_HEIGHT) {
      for (uint16_t i = 0; i < LINE_BUF_SZ; i++ ) {
        dac_buff[buf_idx][i] = luminance[1];
        opamp_buff[buf_idx][i] = video_source;
        IF_USE_COLOR(phase_buff[buf_idx][i] = phase[1]);
      }
    } else {
      uint8_t *line_ptr = video_frame_buffer[active_video_buffer][line - minRenderLine];

      uint32_t buf_idx_local = 0;
      uint32_t word;
      uint8_t* byte = (uint8_t*)&word;

      #ifdef USE_COLOR
      if (show_test_pattern && (line < minRenderLine + 101)) {
          luminance = &video_levels[8];
          opamp = &opa_vals[8];
          phase = &phase_val[palPhase][8];
      }
      #endif

      dac_buff[buf_idx][buf_idx_local] = video_levels[1];
      IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[1]);
      opamp_buff[buf_idx][buf_idx_local++] = opa_vals[1];

      for (uint32_t i = 0; i < VIDEO_BYTES_PER_LINE; i += VIDEO_BPP) {
          #if VIDEO_BPP > 3
          byte[3] = line_ptr[i + VIDEO_BPP - 4];
          #endif
          #if VIDEO_BPP > 2
          byte[2] = line_ptr[i + VIDEO_BPP - 3];
          #endif
          #if VIDEO_BPP > 1
          byte[1] = line_ptr[i + VIDEO_BPP - 2];
          #endif
          byte[0] = line_ptr[i + VIDEO_BPP - 1];
          
          uint8_t pixel;
      
          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (word >> (VIDEO_BPP * 7)) & BITMASK(VIDEO_BPP);
          dac_buff[buf_idx][buf_idx_local] = luminance[pixel];
          IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[pixel]);
          opamp_buff[buf_idx][buf_idx_local++] = opamp[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (word >> (VIDEO_BPP * 6)) & BITMASK(VIDEO_BPP);
          dac_buff[buf_idx][buf_idx_local] = luminance[pixel];
          IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[pixel]);
          opamp_buff[buf_idx][buf_idx_local++] = opamp[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (word >> (VIDEO_BPP * 5)) & BITMASK(VIDEO_BPP);
          dac_buff[buf_idx][buf_idx_local] = luminance[pixel];
          IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[pixel]);
          opamp_buff[buf_idx][buf_idx_local++] = opamp[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (word >> (VIDEO_BPP * 4)) & BITMASK(VIDEO_BPP);
          dac_buff[buf_idx][buf_idx_local] = luminance[pixel];
          IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[pixel]);
          opamp_buff[buf_idx][buf_idx_local++] = opamp[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (word >> (VIDEO_BPP * 3)) & BITMASK(VIDEO_BPP);
          dac_buff[buf_idx][buf_idx_local] = luminance[pixel];
          IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[pixel]);
          opamp_buff[buf_idx][buf_idx_local++] = opamp[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (word >> (VIDEO_BPP * 2)) & BITMASK(VIDEO_BPP);
          dac_buff[buf_idx][buf_idx_local] = luminance[pixel];
          IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[pixel]);
          opamp_buff[buf_idx][buf_idx_local++] = opamp[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = (word >> (VIDEO_BPP * 1)) & BITMASK(VIDEO_BPP);
          dac_buff[buf_idx][buf_idx_local] = luminance[pixel];
          IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[pixel]);
          opamp_buff[buf_idx][buf_idx_local++] = opamp[pixel];

          if (buf_idx_local >= LINE_BUF_SZ) break;
          pixel = word & BITMASK(VIDEO_BPP);
          dac_buff[buf_idx][buf_idx_local] = luminance[pixel];
          IF_USE_COLOR(phase_buff[buf_idx][buf_idx_local] = phase[pixel]);
          opamp_buff[buf_idx][buf_idx_local++] = opamp[pixel];
      }
      dac_buff[buf_idx][LINE_BUF_SZ-1] = video_levels[1];
      opamp_buff[buf_idx][LINE_BUF_SZ-1] = video_source;
    }
}

#else

#if defined(USE_COLOR)

static const uint8_t colorMatrix[4][16] = { {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,2},
                                            {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,4},
                                            {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,7},
                                            {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,5}
                                          };

EXEC_RAM static void squash_canvas_raw_pixel_buff(uint16_t c, uint32_t glyph_row, uint32_t x_off)
{
    uint8_t font_page = (c>>8) & 0x03;

    const uint16_t * const glyph = &font_data[(uint8_t)c * FONT_STRIDE / 2];
    const uint32_t* phase = &phase_val[palPhase][0];

    uint16_t buf_index = x_off;
    uint32_t word_index = glyph_row * BYTES_PER_ROW / 2;
    uint8_t pixel;
    uint16_t word;

    for(uint32_t col = 0; col < FONT_WIDTH / 4; col++) {
        word = glyph[word_index++];
        
        pixel = colorMatrix[font_page][(word >> 12) & 0x0f];
        dac_buff[buf_idx][buf_index] = video_levels[pixel];
        phase_buff[buf_idx][buf_index] = phase[pixel];
        opamp_buff[buf_idx][buf_index++] = opa_vals[pixel];

        pixel = colorMatrix[font_page][(word >> 8) & 0x0f];
        dac_buff[buf_idx][buf_index] = video_levels[pixel];
        phase_buff[buf_idx][buf_index] = phase[pixel];
        opamp_buff[buf_idx][buf_index++] = opa_vals[pixel];

        pixel = colorMatrix[font_page][(word >> 4) & 0x0f];
        dac_buff[buf_idx][buf_index] = video_levels[pixel];
        phase_buff[buf_idx][buf_index] = phase[pixel];
        opamp_buff[buf_idx][buf_index++] = opa_vals[pixel];

        pixel = colorMatrix[font_page][(word >> 0) & 0x0f];
        dac_buff[buf_idx][buf_index] = video_levels[pixel];
        phase_buff[buf_idx][buf_index] = phase[pixel];
        opamp_buff[buf_idx][buf_index++] = opa_vals[pixel];
    }
}

#else

EXEC_RAM static void squash_canvas_raw_pixel_buff(char c, uint32_t glyph_row, uint32_t x_off)
{
    const uint8_t * const glyph = &font_data[(uint8_t)c * FONT_STRIDE];

    uint16_t buf_index = x_off;
    uint32_t byte_index = glyph_row * BYTES_PER_ROW;
    uint8_t pixel;
    uint8_t byte;

    for(uint32_t col = 0; col < FONT_WIDTH / 4; col++) {
        byte = glyph[byte_index++];
        
        pixel = (byte >> 6) & 0x03;
        dac_buff[buf_idx][buf_index] = video_levels[pixel];
        opamp_buff[buf_idx][buf_index++] = opa_vals[pixel];

        pixel = (byte >> 4) & 0x03;
        dac_buff[buf_idx][buf_index] = video_levels[pixel];
        opamp_buff[buf_idx][buf_index++] = opa_vals[pixel];

        pixel = (byte >> 2) & 0x03;
        dac_buff[buf_idx][buf_index] = video_levels[pixel];
        opamp_buff[buf_idx][buf_index++] = opa_vals[pixel];

        pixel = (byte >> 0) & 0x03;
        dac_buff[buf_idx][buf_index] = video_levels[pixel];
        opamp_buff[buf_idx][buf_index++] = opa_vals[pixel];
    }
}
#endif

void render_overlay_logo_line(uint16_t line, uint8_t frame)
{
    if (line >= maxRenderLine) return;

    // Logo line considering vertical offset

    #ifdef USE_HD
    if (((line<<1) + frame) < LOGO_OFFSET_Y * 3 || (((line<<1) + frame) - LOGO_OFFSET_Y * 3) >= LOGO_HEIGHT) {
        // Line outside the logo area — do nothing, keep existing buffer contents
        return;
    }
    uint16_t logo_row = ((line<<1) + frame) - LOGO_OFFSET_Y * 3;
    #else
    UNUSED(frame);
    if (line < LOGO_OFFSET_Y || (line - LOGO_OFFSET_Y) >= LOGO_HEIGHT) {
        // Line outside the logo area — do nothing, keep existing buffer contents
        return;
    }
    uint16_t logo_row = line - LOGO_OFFSET_Y;
    #endif

    const uint8_t *logo_line_ptr = &logo_data[logo_row * LOGO_ROW_BYTES];

    uint32_t buf_idx_local = (PIXELS_PER_LINE -LOGO_WIDTH) / 2;
    for (uint32_t i = 0; i < LOGO_WIDTH>>2; i++) {
        uint8_t byte = logo_line_ptr[i];
        uint8_t pixel;
        
        if (buf_idx_local >= LINE_BUF_SZ) break;
        pixel = (byte >> 6) & 0x3;
        dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
        opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

        if (buf_idx_local >= LINE_BUF_SZ) break;
        pixel = (byte >> 4) & 0x3;
        dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
        opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

        if (buf_idx_local >= LINE_BUF_SZ) break;
        pixel = (byte >> 2) & 0x3;
        dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
        opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];

        if (buf_idx_local >= LINE_BUF_SZ) break;
        pixel = byte & 0x3;
        dac_buff[buf_idx][buf_idx_local] = video_levels[pixel];
        opamp_buff[buf_idx][buf_idx_local++] = opa_vals[pixel];
    }
}

EXEC_RAM static void render_line(uint16_t line, uint8_t frame)
{
    CCMRAM_BSS static uint32_t draw_line = 0;
    uint32_t map_row = 0;
    uint32_t glyph_row = 0;
    uint32_t i = 0;
    uint16_t c = 0;

    // Offset current draw line by Y_OFFSET
    draw_line = line - minRenderLine;

    // Calculate which character row and which row in the glyph
    #ifdef USE_HD
    map_row    = ((draw_line<<1) + frame) / FONT_HEIGHT;  // 0..ROW_SIZE-1
    glyph_row  = ((draw_line<<1) + frame) % FONT_HEIGHT;  // 0..FONT_HEIGHT-1
    #else
    UNUSED(frame);
    map_row    = draw_line / FONT_HEIGHT;  // 0..ROW_SIZE-1
    glyph_row  = draw_line % FONT_HEIGHT;  // 0..FONT_HEIGHT-1
    #endif

    if (line < minRenderLine || map_row >= ROW_SIZE) {
        // Out of screen — just transparent
        for (i = 0; i < LINE_BUF_SZ; i++) {
            dac_buff[buf_idx][i] = video_levels[1];
            opamp_buff[buf_idx][i] = video_source;
        }
        return;
    }

    dac_buff[buf_idx][0] = video_levels[1];
    opamp_buff[buf_idx][0] = video_source;
    
    #ifdef USE_COLOR
    uint32_t* phase;
    phase = &phase_val[palPhase][0];
    phase_buff[buf_idx][0] = phase[2];
    #endif

    // Render each character of the map
    for (i = 0; i < COLUMN_SIZE; i++) {
        c = canvas_char_map[active_buffer][map_row][i];
        squash_canvas_raw_pixel_buff(c, glyph_row, 1 + i * FONT_WIDTH);
    }

    opamp_buff[buf_idx][LINE_BUF_SZ-1] = video_source;
}
#endif

#if 0
void render_test_pattern_line(uint16_t line)
{
    if (line >= 105) return;

    #ifdef USE_COLOR
    uint32_t* phase;
    phase = phase_val[palPhase];
    #endif

    if (line < 50 || line >= 120) {
      for (uint16_t i = 0; i < LINE_BUF_SZ; i++ ) {
        IF_USE_COLOR(phase_buff[buf_idx][i] = phase[1]);
        dac_buff[buf_idx][i] = video_levels[1];
        opamp_buff[buf_idx][i] = video_source;
      }
    } else {

      uint16_t buf_index;

      for (buf_index = 0; buf_index < 50; buf_index++ ) {
        IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[1]);
        dac_buff[buf_idx][buf_index] = video_levels[1];
        opamp_buff[buf_idx][buf_index] = video_source;
        
      }

      for(uint8_t  y=4; y<8; y++) {
        for (uint8_t i = 0; i<8 ; i++) {
          IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[y]);
          dac_buff[buf_idx][buf_index] = video_levels[y];
          opamp_buff[buf_idx][buf_index++] = opa_vals[y];
          
          IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[y]);
          dac_buff[buf_idx][buf_index] = video_levels[y];
          opamp_buff[buf_idx][buf_index++] = opa_vals[y];

          IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[y]);
          dac_buff[buf_idx][buf_index] = video_levels[y];
          opamp_buff[buf_idx][buf_index++] = opa_vals[y];

          IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[y]);
          dac_buff[buf_idx][buf_index] = video_levels[y];
          opamp_buff[buf_idx][buf_index++] = opa_vals[y];
        }
      }

      for (uint8_t i = 0; i<8 && buf_index < LINE_BUF_SZ ; i++) {
        IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[0]);
        dac_buff[buf_idx][buf_index] = video_levels[0];
        opamp_buff[buf_idx][buf_index++] = opa_vals[0];

        IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[0]);
        dac_buff[buf_idx][buf_index] = video_levels[0];
        opamp_buff[buf_idx][buf_index++] = opa_vals[0];

        IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[0]);
        dac_buff[buf_idx][buf_index] = video_levels[0];
        opamp_buff[buf_idx][buf_index++] = opa_vals[0];

        IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[0]);
        dac_buff[buf_idx][buf_index] = video_levels[0];
        opamp_buff[buf_idx][buf_index++] = opa_vals[0];
      }
      
      for (; buf_index < LINE_BUF_SZ; buf_index++ ) {
        IF_USE_COLOR(phase_buff[buf_idx][buf_index] = phase[1]);
        dac_buff[buf_idx][buf_index] = video_levels[1];
        opamp_buff[buf_idx][buf_index] = video_source;
      }

      IF_USE_COLOR(phase_buff[buf_idx][LINE_BUF_SZ-1] = phase[1]);
      dac_buff[buf_idx][LINE_BUF_SZ-1] = video_levels[1];
      opamp_buff[buf_idx][LINE_BUF_SZ-1] = video_source;
    }
}
#endif

EXEC_RAM static void push_line_to_dma(uint16_t line, uint8_t frame)
{
    #if DMA_DOUBLE_BUFFER == 0
      buf_idx = 0;
    #else
      buf_idx = (line & 1);
    #endif
    
    // Stop TIM1 and DMA channels
    LL_TIM_DisableCounter(TIM1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
    LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_1);
    #ifdef USE_COLOR
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_8);
    #endif

    // Configure length and addresses
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)&opamp_buff[buf_idx ^ DMA_DOUBLE_BUFFER][0]); // ! send previous buffer
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, LINE_BUF_SZ);
    LL_DMA_SetMemoryAddress(DMA2, LL_DMA_CHANNEL_1, (uint32_t)&dac_buff[buf_idx ^ DMA_DOUBLE_BUFFER][0]); // ! send previous buffer
    LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_1, (LINE_BUF_SZ));

    // Enable DMA request for DAC
    LL_DAC_EnableDMAReq(DAC3, LL_DAC_CHANNEL_1);

    if ((line > minRenderLine - (1 - DMA_DOUBLE_BUFFER))) {
      // Enable DMA channels
      LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_1); // first start DAC channel
      LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);

      #ifdef USE_COLOR
      if(syncState == SYNC_STATE_EXTERNAL) {
        LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_8, (uint32_t)&phase_buff[buf_idx ^ DMA_DOUBLE_BUFFER][(colorDelay / TIM1_AUTORELOAD ) & 0x07]); // ! send previous buffer
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_8, LINE_BUF_SZ);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_8);
      }
      #endif

      LL_TIM_EnableDMAReq_UPDATE(TIM1);
      LL_TIM_EnableDMAReq_CC1(TIM1);
      LL_TIM_EnableDMAReq_CC2(TIM1);
    }

#if defined(USE_GRAPHICS)
    UNUSED(frame);
    render_video_line(line); // video frame buffer
#else
    render_line(line, frame); // char canvas map
    if (show_test_pattern) {
      //render_test_pattern_line(line);
    } else if (show_logo) {
      render_overlay_logo_line(line, frame);  
    }
#endif
}


EXEC_RAM static inline void pars_video_signal(uint32_t tim_tick)
{
    CCMRAM_BSS static uint16_t video_line = 0;
    CCMRAM_BSS static uint16_t videoLineFull = 0;
    CCMRAM_BSS static uint8_t vsync = 0;
    static uint8_t halfLine = 0;
    static uint8_t frame = 0;

    register float time_ns = (float)tim_tick * TIM2_TICK_MS;
    if(time_ns > 59.f && time_ns < 67.5f) {
        video_line++;
        videoLineFull++;

        #ifdef TRIGGER_LINE
        led_set(TP2,videoLineFull == triggerLine);
        #endif
        if (video_line <= maxRenderLine) {
          if(syncState == SYNC_STATE_EXTERNAL) {
            #ifdef USE_COLOR
            // Detach line sync from COMP2
            LL_TIM_SetETRSource(TIM2, LL_TIM_TIM2_ETRSOURCE_COMP3);
            LL_TIM_SetRemap(TIM2, LL_TIM_TIM2_TI2_RMP_COMP3);
            // Open trigger gate for HRTIM TIMER B colorbust synchronisation
            LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, DAC12BIT_FROM_MV(sync_voltage_black - 15) * videoInputs[activeVideoInput].gain);
            LL_HRTIM_TIM_SetResetTrig(HRTIM1, LL_HRTIM_TIMER_B, LL_HRTIM_RESETTRIG_EEV_1);
            #endif
          }
          push_line_to_dma(video_line, frame);
        } else if (video_line == maxRenderLine + 1) {
          RGB_led_send();
        }
        
        if (new_field == false) {
            new_field = true;
        }
        vsync = 0;

    } else if (time_ns > 29.0f && time_ns < 35.0f) {
        vsync++;
        halfLine++;
        if(!(halfLine & 0x01)) {
          videoLineFull++;
          #ifdef TRIGGER_LINE
          led_set(TP2,videoLineFull == triggerLine);
          #endif
        }
        
        if (vsync == 5) {
          sync_voltage_low = DAC12BIT_TO_MV(LL_ADC_INJ_ReadConversionData12(ADC1,LL_ADC_INJ_RANK_1) / VIDEO_TOTAL_GAIN);
          LL_TIM_OC_SetCompareCH1(TIM2, NS_TO_TICKS(BLACK_LEVEL_ADC_DELAY_NS));
        }
        if (vsync == 11) {
          sync_voltage_black = DAC12BIT_TO_MV(LL_ADC_INJ_ReadConversionData12(ADC1,LL_ADC_INJ_RANK_1) / VIDEO_TOTAL_GAIN);
          
          if((syncState == SYNC_STATE_EXTERNAL) && (sync_voltage_black > sync_voltage_low) && (sync_voltage_black - sync_voltage_low > 100)) {
            sync_voltage = sync_voltage_black - ((sync_voltage_black - sync_voltage_low) / 2);
            LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, DAC12BIT_FROM_MV(sync_voltage) * videoInputs[activeVideoInput].gain);
          }
        }

    } else if (time_ns > 56.0f && time_ns < 58.0f) {
        if (vsync >= 4) {
          if(vsync == 5 ) {
            videoLineFull = 1;
            frame = 1;
            halfLine = 1;
          } else {
            videoLineFull++;
            frame = 0;
          }
          #ifdef TRIGGER_LINE
          led_set(TP2,videoLineFull == triggerLine);
          #endif

          vsync = 4;
          LL_TIM_OC_SetCompareCH1(TIM2, NS_TO_TICKS(LOW_SYNC_ADC_DELAY_NS));
        }

    } else if (time_ns > 6.5f && time_ns < 7.5f) {
        if ((videoMode == MODE_PAL && vsync == 8) || (videoMode == MODE_NTSC && vsync == 9)) {
          
          if (new_field == true) {
            new_field = false;
          }

          frame_counter++;
          video_line = 0;
        }

    } else {
        vsync = 0;
        // Do nothing, wait for next sync
    }

    #ifdef TRIGGER_LINE
      //led_set(TP2,0);
    #endif
}

EXEC_RAM static inline void check_resync(uint32_t tim_tick)
{
    CCMRAM_BSS static uint8_t vsync = 0;

    register float time_ns = (float)tim_tick * TIM2_TICK_MS;
    if (time_ns > 29.0f && time_ns < 33.5f) {
        vsync++;
    } else if (time_ns > 56.0f && time_ns < 58.0f) {
        if (vsync >= 4) {
          vsync = 4;
        }
    } else if (time_ns > 6.5f && time_ns < 7.5f) {
        if (vsync == 8) {
          set_video_mode(MODE_PAL);
        } else if (vsync == 9) {
          set_video_mode(MODE_NTSC);
        }else {
          vsync = 0;
        }

    } else if ((time_ns > 33.5f && time_ns < 35.5f) || (time_ns > 65.5f && time_ns < 67.5f)) {
        if (videoMode == MODE_PAL && vsync == 12) {
          syncState = SYNC_STATE_FOUND;
        } else if (videoMode == MODE_NTSC && vsync == 14) {
          syncState = SYNC_STATE_FOUND;
        }
        vsync = 0;

    } else {
        vsync = 0;
    }
}


// Called from COMP2 & TIM2 event (video input)
EXEC_RAM void TIM2_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_CC2(TIM2)) {
      if (syncMode == EXTERNAL || syncMode == AUTOMATIC) {
          if (syncState == SYNC_STATE_SEARCH) {
            check_resync(TIM2->CCR2);
          }

          if (syncState == SYNC_STATE_FOUND) {
            if (video_gen_enabled == true) {
                video_gen_stop();
            }
            TRACE_INFO("Sync found\n");
            #ifdef USE_COLOR
            set_color_system(videoMode);
            set_color_phase(videoMode);
            #endif
            set_video_source(videoInputs[activeVideoInput].opampInput);
            syncState = SYNC_STATE_EXTERNAL;

          } else if (syncState == SYNC_STATE_EXTERNAL) {
              //led_set(TP1,1);
              pars_video_signal(TIM2->CCR2);
              //led_set(TP1,0);

              set_black_level(LL_ADC_INJ_ReadConversionData12(ADC1,LL_ADC_INJ_RANK_1) / VIDEO_TOTAL_GAIN);
          }
      }
      LL_TIM_ClearFlag_CC2(TIM2);
    }

    if (LL_TIM_IsActiveFlag_CC3(TIM2)) {
      // Stop pixel output
      if(syncState == SYNC_STATE_EXTERNAL) {
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_1);
        LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_1);
        #ifdef USE_COLOR
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_8);
        #endif
        OPAMP1->CSR = video_source;
      }
      
      #ifdef USE_COLOR
      if(syncState == SYNC_STATE_EXTERNAL) {
        // Attach line sync to COMP2
        LL_DAC_ConvertData12RightAligned(DAC3, LL_DAC_CHANNEL_2, DAC12BIT_FROM_MV(sync_voltage) * videoInputs[activeVideoInput].gain);
        LL_TIM_SetETRSource(TIM2, LL_TIM_TIM2_ETRSOURCE_COMP2);
        LL_TIM_SetRemap(TIM2, LL_TIM_TIM2_TI2_RMP_COMP2);
      }
      #endif
      
      LL_TIM_ClearFlag_CC3(TIM2);
    }
}

// Called form TIM17 event (video gen)
EXEC_RAM void TIM1_TRG_COM_TIM17_IRQHandler(void)
{
    static uint32_t ARR_prev = 0;
    if (LL_TIM_IsActiveFlag_UPDATE(TIM17)) {
      if(DMA1_Channel5->CNDTR == 1) {
        ARR_prev += TIM17->ARR;
      } else {
        if (video_gen_enabled == true) {
            pars_video_signal(ARR_prev);
        }
        ARR_prev = TIM17->ARR;
      }
      LL_TIM_ClearFlag_UPDATE(TIM17);
    }
}


void video_sync_loop(void) {
  static uint32_t last_tick = 0;
  static uint8_t last_frame;

  if (syncMode <= INTERNAL) {
    return;
  }

  if (((HAL_GetTick() - last_tick) >= 25) || (frame_counter != last_frame)) {

    if(syncState == SYNC_STATE_SEARCH) {
      scan_sync_voltage();
    } 

    if(syncState == SYNC_STATE_EXTERNAL) {
      if(last_frame == frame_counter) {
        if(!--sync_lost) {
          TRACE_INFO("Sync lost\n");
          syncState = SYNC_STATE_SEARCH;
          if (syncMode == AUTOMATIC) {
            video_gen_start();
            LL_TIM_SetETRSource(TIM2, LL_TIM_TIM2_ETRSOURCE_COMP2);
            LL_TIM_SetRemap(TIM2, LL_TIM_TIM2_TI2_RMP_COMP2);
            set_video_source(OPAMP_CONST_DAC);
            set_black_level(DAC_BLACK);
          }
          sync_lost = SYNC_LOST_FRAMES_THRESHOLD;
        }
      } else {
        sync_lost = SYNC_LOST_FRAMES_THRESHOLD;
      }
      
    }
    last_frame = frame_counter;
    last_tick = HAL_GetTick();
  }
}

void setSyncMode(syncMode_t mode) {

  switch(mode) {
    case AUTOMATIC:

      syncMode = AUTOMATIC;
      break;
    case EXTERNAL:

      syncMode = EXTERNAL;
      break;
    case INTERNAL:
      video_gen_start();
      LL_TIM_SetETRSource(TIM2, LL_TIM_TIM2_ETRSOURCE_COMP2);
      LL_TIM_SetRemap(TIM2, LL_TIM_TIM2_TI2_RMP_COMP2);
      set_video_source(OPAMP_CONST_DAC);
      set_black_level(DAC_BLACK);
      syncMode = INTERNAL;
      break;
    case OFF:
      if (video_gen_enabled == true) {
          video_gen_stop();
      }
      set_video_source(videoInputs[activeVideoInput].opampInput);
      syncMode = OFF;
      break;
  }

}

#if (VIDEO1_INPUT_ENABLED == true && VIDEO2_INPUT_ENABLED == true)
void cameraControl1_changed(bool value) {
  if(value) {
    TRACE_INFO("CAM SWITCH on\n");
    if (settings.camswitchEnabled)
      set_video_input(1);
  } else {
    
    TRACE_INFO("CAM SWITCH off\n");
    if (settings.camswitchEnabled)
      set_video_input(0);
  }
}
#endif