/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "update_font.h"
#include "main.h"
#include "stm32g4xx.h"

#include <string.h>

#define FLASH_PAGE_SIZE         0x800U // 2 КБ
#define FLASH_BANK1_BASE        0x08000000U
#define FLASH_FONT_ADDRESS      0x0801C000U // 0x0801C000 = 0x08000000 + 56 * 0x800
#define FLASH_FONT_SIZE         0x4000U // 16 КБ
#define FONT_CHARS_SIZE         64U // 64 bytes per char
#define FONT_CHARS_COUNT        256U // 256 charters (0x00 - 0xFF)

#define FLASH_FONT_START_PAGE   ((FLASH_FONT_ADDRESS - FLASH_BANK1_BASE) / FLASH_PAGE_SIZE) // 56
#define FLASH_FONT_NB_PAGES     (FLASH_FONT_SIZE / FLASH_PAGE_SIZE) // 8


#if 0 // for debug TODO: remove later
int flash_unlock(void)
{
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;
    }
    if (FLASH->CR & FLASH_CR_LOCK) {
        return -1;
    }
    return 0;
}

void flash_lock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}
#endif

int update_font_erase_all(void)
{
    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t sectorError = 0;

    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return UPDATE_FONT_ERR_FLASH_UNLOCK;

    eraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInitStruct.Banks = FLASH_BANK_1;
    eraseInitStruct.Page = FLASH_FONT_START_PAGE; // 56
    eraseInitStruct.NbPages = FLASH_FONT_NB_PAGES; // 8

    status = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return UPDATE_FONT_ERR_FLASH_ERASE;
    }

    HAL_FLASH_Lock();
    return UPDATE_FONT_OK;
}

int update_font_symbol_write(uint8_t char_index, const uint8_t *data_64bytes, size_t data_size)
{
    if (data_size != FONT_CHARS_SIZE || data_64bytes == NULL) return UPDATE_FONT_ERR_INVALID_SIZE;

    uint32_t char_offset = char_index * FONT_CHARS_SIZE;
    if (char_offset + FONT_CHARS_SIZE > FLASH_FONT_SIZE) return UPDATE_FONT_ERR_OUT_OF_RANGE;

    // Address of the char in flash
    uint32_t char_flash_addr = FLASH_FONT_ADDRESS + char_offset;

    // Determine the base address of the flash page where the symbol is located
    uint32_t page_start_addr = char_flash_addr & ~(FLASH_PAGE_SIZE - 1);

   // Buffer for copying a flash page (2 KB)
    uint8_t page_buffer[FLASH_PAGE_SIZE]; // TODO: test: maybe use global variable instead of stack?

    // Reading the entire page into RAM
    memcpy(page_buffer, (void *)page_start_addr, FLASH_PAGE_SIZE);

   // Position of the symbol within the page (offset from the start of the page)
    uint32_t offset_in_page = char_flash_addr - page_start_addr;

   // Copy 64 bytes of the symbol into the page buffer
    memcpy(&page_buffer[offset_in_page], data_64bytes, FONT_CHARS_SIZE);

    // Unlocking flash memory
    HAL_StatusTypeDef status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return UPDATE_FONT_ERR_FLASH_UNLOCK;

   // Erase the flash page
    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t sectorError = 0;
    eraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInitStruct.Banks = FLASH_BANK_1;
    eraseInitStruct.Page = (page_start_addr - FLASH_BANK1_BASE) / FLASH_PAGE_SIZE;
    eraseInitStruct.NbPages = 1;

    status = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return UPDATE_FONT_ERR_FLASH_ERASE;
    }

    // Write the page back in 64-bit (double word) chunks
    uint32_t addr = page_start_addr;
    for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 8) {
        uint64_t data64 = *(uint64_t *)(page_buffer + i);
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, data64);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return UPDATE_FONT_ERR_FLASH_WRITE;
        }
        addr += 8;
    }

    HAL_FLASH_Lock();

    return UPDATE_FONT_OK;
}
