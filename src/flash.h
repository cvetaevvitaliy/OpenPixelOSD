/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef FLASH_H
#define FLASH_H
#include <stdint.h>
#include <stdbool.h>

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
