/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef CANVAS_CHAR_H
#define CANVAS_CHAR_H
#include <stdint.h>
#include "main.h"

#if defined(USE_COLOR)
typedef uint16_t canvasChar_t;
#else
typedef char canvasChar_t;
#endif

extern canvasChar_t canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];

EXEC_RAM void canvas_char_flush_map(void);
EXEC_RAM void canvas_char_clean(void);
EXEC_RAM void canvas_char_write(uint8_t x, uint8_t y, const char *data, const uint16_t len, uint8_t font);
EXEC_RAM void canvas_char_draw_complete(void);
EXEC_RAM void canvas_print(uint8_t x, uint8_t y, const char *str);


#endif //CANVAS_CHAR_H
