/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>
#include <stdbool.h>

// header(1) + freq table(3) + up to VTX_POWER_LEVEL_MAX(16) levels x 6 blocks/level + settings(1)= 101
#define FLASH_EEPROM_NB_BLOCKS    101

typedef struct {
    uint8_t idx;
    uint8_t value[7];
} flashBlock_t;

bool eeprom_read(flashBlock_t* val);
void eeprom_write(flashBlock_t* val);
void eeprom_save(void);
void eeprom_dump(void);
void flash_init(void);

#endif //FLASH_H
