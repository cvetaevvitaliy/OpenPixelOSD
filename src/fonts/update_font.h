/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef UPDATE_FONT_H
#define UPDATE_FONT_H
#include <stddef.h>
#include <stdint.h>

typedef enum {
  UPDATE_FONT_OK                = 0,  // Success
  UPDATE_FONT_ERR_INVALID_SIZE = -1,  // Invalid data size or NULL pointer
  UPDATE_FONT_ERR_OUT_OF_RANGE = -2,  // Symbol index is out of font range
  UPDATE_FONT_ERR_FLASH_UNLOCK = -3,  // Flash unlock error
  UPDATE_FONT_ERR_FLASH_ERASE  = -4,  // Flash page erase error
  UPDATE_FONT_ERR_FLASH_WRITE  = -5   // Flash page write error
} UpdateFontError_t;

int update_font_erase_all(void);
int update_font_symbol_write(uint8_t char_index, const uint8_t *data_64bytes, size_t data_size);

#endif //UPDATE_FONT_H
