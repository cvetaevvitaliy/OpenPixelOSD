/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdbool.h>
#include <string.h>
#include "canvas_char.h"
#include "main.h"


CCMRAM_BSS char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
CCMRAM_DATA uint8_t active_buffer = 0;
CCMRAM_DATA static bool canvas_write_next_buff = false;

EXEC_RAM void canvas_char_flush_map(void)
{
    memset(canvas_char_map, ' ', sizeof(canvas_char_map));
    canvas_write_next_buff = true;
}

EXEC_RAM void canvas_char_clean(void)
{
    memset(canvas_char_map[active_buffer], ' ', sizeof(canvas_char_map[0]));
    if (!canvas_write_next_buff) {
        canvas_write_next_buff = true; // Set flag to switch buffer on next draw
    }
}

EXEC_RAM void canvas_char_write(uint8_t x, uint8_t y, const char *data, const uint16_t len)
{
    if (y >= ROW_SIZE) y = 0;
    if (x >= COLUMN_SIZE) x = 0;

    for (uint16_t i = 0; i < len; i++) {
        const uint8_t col = (x + i) % COLUMN_SIZE;
        canvas_char_map[active_buffer][y][col] = data[i];
    }
}

EXEC_RAM void canvas_char_draw_complete(void)
{
    if (canvas_write_next_buff) {
        active_buffer ^= 1; // Switch active buffer
        canvas_write_next_buff = false;
    }
}
