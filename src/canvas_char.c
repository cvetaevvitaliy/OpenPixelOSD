/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdbool.h>
#include <string.h>
#include "canvas_char.h"
#include "main.h"

#if defined(USE_GRAPHICS)
#include "video_graphics.h"

#define FONT_WIDTH      (12)
#define FONT_HEIGHT     (18)

EXEC_RAM void canvas_char_flush_map(void)
{
    
}

EXEC_RAM void canvas_char_clean(void)
{
    video_graphics_clear_draw_buff(PX_TRANSPARENT);
}

EXEC_RAM void canvas_char_write(uint8_t x, uint8_t y, const char *data, const uint16_t len, uint8_t font)
{
    if (y >= ROW_SIZE) y = 0;
    if (x >= COLUMN_SIZE) x = 0;

    for (uint16_t i = 0; i < len; i++) {
        const uint8_t col = (x + i) % COLUMN_SIZE;
        video_draw_char_at(data[i], col * FONT_WIDTH, y * FONT_HEIGHT, font);
    }
}

EXEC_RAM void canvas_char_draw_complete(void)
{
    video_graphics_draw_complete();
}

EXEC_RAM void canvas_print(uint8_t x, uint8_t y, const char *str) {
  if (x >= COLUMN_SIZE) return;
  if (y >= ROW_SIZE) return;

  while (*str && x < COLUMN_SIZE) {
    video_draw_char_at(*str++, x++ * FONT_WIDTH, y * FONT_HEIGHT, PX_WHITE);
  }
}


#else
CCMRAM_BSS canvasChar_t canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
CCMRAM_DATA uint8_t active_buffer = 0;
CCMRAM_DATA uint8_t paint_buffer = 1;

EXEC_RAM void canvas_char_flush_map(void)
{
    memset(canvas_char_map, ' ', sizeof(canvas_char_map));
}

EXEC_RAM void canvas_char_clean(void)
{
    paint_buffer = 1 - active_buffer;
    memset(canvas_char_map[paint_buffer], ' ', sizeof(canvas_char_map[0]));
}

EXEC_RAM void canvas_char_write(uint8_t x, uint8_t y, const char *data, const uint16_t len, uint8_t font)
{
    UNUSED(font);
    
    if (y >= ROW_SIZE) y = 0;
    if (x >= COLUMN_SIZE) x = 0;

    for (uint16_t i = 0; i < len; i++) {
        const uint8_t col = (x + i) % COLUMN_SIZE;
        canvas_char_map[paint_buffer][y][col] = data[i] | (font<<8);
    }
}

EXEC_RAM void canvas_char_draw_complete(void)
{
    active_buffer = paint_buffer; // Switch active buffer 
}

EXEC_RAM void canvas_print(uint8_t x, uint8_t y, const char *str) {
  if (x >= COLUMN_SIZE) return;
  if (y >= ROW_SIZE) return;

  while (*str && x < COLUMN_SIZE) {
    canvas_char_map[paint_buffer][y][x++] = *str++;
  }
}


#endif
