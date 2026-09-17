/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "flash.h"
#include "main.h"
#include <stdbool.h>
#include <string.h>

extern uint8_t __eeprom_start[];
extern uint8_t __eeprom_size[];

#define FLASH_PAGE_SIZE           0x800U
#define FLASH_BANK1_BASE          0x08000000U

#define FLASH_EEPROM_ADDRESS      ((uint32_t)&__eeprom_start)
#define FLASH_EEPROM_SIZE         ((uint32_t)&__eeprom_size)
#define FLASH_EEPROM_START_PAGE   ((FLASH_EEPROM_ADDRESS - FLASH_BANK1_BASE) / FLASH_PAGE_SIZE)
#define FLASH_EEPROM_NB_PAGES     (FLASH_EEPROM_SIZE / FLASH_PAGE_SIZE)
#define FLASH_BANK_NUMBER(addr)   (((addr) < (FLASH_BANK1_BASE + 0x40000U)) ? 1U : 2U)
#define FLASH_BLOCKS_PER_PAGE     (FLASH_PAGE_SIZE / sizeof(flashBlock_t))
#define FLASH_EEPROM_NB_BLOCKS    32

#define FLASH_INIT_DOUBLEWORD     (0xdeadbeff)

flashBlock_t eeprom[FLASH_EEPROM_NB_BLOCKS];

bool flash_erase() {
  // Unlocking flash memory
  HAL_StatusTypeDef status = HAL_FLASH_Unlock();
  if (status != HAL_OK) return false;

  // Erase the flash page
  FLASH_EraseInitTypeDef eraseInitStruct;
  uint32_t sectorError = 0;
  eraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
  eraseInitStruct.Banks = FLASH_BANK_NUMBER(FLASH_EEPROM_ADDRESS);
  eraseInitStruct.Page = FLASH_EEPROM_START_PAGE;
  eraseInitStruct.NbPages = FLASH_EEPROM_NB_PAGES;

  status = HAL_FLASHEx_Erase(&eraseInitStruct, &sectorError);
  if (status != HAL_OK) {
      HAL_FLASH_Lock();
      return 0;
  }

  // Write the page back in 64-bit (double word) chunks
  status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FLASH_EEPROM_ADDRESS, FLASH_INIT_DOUBLEWORD);
  if (status != HAL_OK) {
      HAL_FLASH_Lock();
      return 0;
  }

  HAL_FLASH_Lock();
  return 1;
}

flashBlock_t* flash_seek(uint8_t idx) {
  uint16_t x = (FLASH_BLOCKS_PER_PAGE * FLASH_EEPROM_NB_PAGES) - 1;
  flashBlock_t* seekVal = (flashBlock_t*)FLASH_EEPROM_ADDRESS;

  do {
    if ((seekVal[x].idx) == idx) {
      return &seekVal[x];;
    }
  } while (x--);

  return NULL;
}

uint16_t flash_free(void) {
  flashBlock_t* flashBlock = (flashBlock_t*)FLASH_EEPROM_ADDRESS;
  uint16_t ret = 0;

  for (uint16_t x = 1; x < (FLASH_BLOCKS_PER_PAGE * FLASH_EEPROM_NB_PAGES); x++) {
    if (*((uint64_t*)&flashBlock[x]) == 0xffffffffffffffff) {
      ret++;
    }
  }

  return ret;
}

uint16_t flash_modified(void) {
  flashBlock_t* flashBlock;
  uint16_t ret = 0;

  for (uint8_t idx = 0; idx < FLASH_EEPROM_NB_BLOCKS; idx++) {
    if (eeprom[idx].idx == idx) {
      flashBlock = flash_seek(idx);
      if ((flashBlock == NULL) || (memcmp(&eeprom[idx], flashBlock, sizeof(flashBlock_t)) != 0)) {
        ret++;
      }
    }
  }

  return ret;
}

uint8_t flash_push(flashBlock_t* block) {
  uint16_t x = 0;
  flashBlock_t* flashBlock = (flashBlock_t*)FLASH_EEPROM_ADDRESS;
  flashBlock_t* foundBlock;

  foundBlock = flash_seek(block->idx);
  if (foundBlock) {
    if (memcmp(foundBlock, block, sizeof(flashBlock_t)) == 0) {
      TRACE_DEBUG("FLASH match %lx: \n",(uint32_t)foundBlock);
      return 1;
    }
  }

  for (x = 1; x < (FLASH_BLOCKS_PER_PAGE * FLASH_EEPROM_NB_PAGES); x++) {
    if (*((uint64_t*)&flashBlock[x]) == 0xffffffffffffffff) {
      break;
    }
  }
  
  if( x == FLASH_BLOCKS_PER_PAGE * FLASH_EEPROM_NB_PAGES) {
    TRACE_DEBUG("FLASH full\n");
    return 0;
  }

  TRACE_DEBUG("FLASH write %i: %lx: ",x ,(uint32_t)&flashBlock[x]);

  for (uint8_t i=0; i<sizeof(flashBlock_t); i++) {
    uint8_t* b = (uint8_t*)block;
    UNUSED(b);
    TRACE_DEBUG_WP("%x ", b[i]);
  }
  

  const uint8_t max_attempts = 5;
  uint8_t attemps = 0;
  HAL_StatusTypeDef status;
  
  __disable_irq();
  HAL_FLASH_Unlock();
  do {
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, (uint32_t)&flashBlock[x], *((uint64_t*)block));
    attemps++;
  } while (status && attemps < max_attempts);
  HAL_FLASH_Lock();
  __enable_irq();

  TRACE_DEBUG_WP("status %i verify: ", status);
  
  for (uint8_t i=0; i<sizeof(flashBlock_t); i++) {
    TRACE_DEBUG_WP("%x ", ((uint8_t*)&flashBlock[x])[i]);
  }
  TRACE_DEBUG_WP("\n");

  return 1;
}

bool eeprom_read(flashBlock_t* val) {
  if(val->idx >= FLASH_EEPROM_NB_BLOCKS) {
    return false;
  }
  
  if (eeprom[val->idx].idx == val->idx) {
    memcpy(val, &eeprom[val->idx], sizeof(flashBlock_t));
    return true;
  }
  return false;

}

void eeprom_write(flashBlock_t* val) {
  if(val->idx >= FLASH_EEPROM_NB_BLOCKS) {
    return;
  }
  memcpy(&eeprom[val->idx] ,val, sizeof(flashBlock_t));

}


void eeprom_save(void) {
  if (flash_modified() > flash_free()) {
    flash_erase();
    TRACE_DEBUG("EEPROM FLASH reorg\n");
  }

  for (uint8_t idx = 0; idx < FLASH_EEPROM_NB_BLOCKS; idx++) {
    if (eeprom[idx].idx == idx) {
      TRACE_DEBUG("EEPROM save block %i\n", idx);
      flash_push((flashBlock_t*)&eeprom[idx]);
    }
  }
}


void eeprom_dump(void) {

  TRACE_DEBUG("");
  for (uint8_t idx = 0; idx < FLASH_EEPROM_NB_BLOCKS; idx++) {
    TRACE_DEBUG_WP("%02x ", eeprom[idx].idx);
    for (uint8_t i=0; i<7; i++) {
      TRACE_DEBUG_WP("%02x ", eeprom[idx].value[i]);
    }
    if ((idx & 0x03) == 0x03) {
      TRACE_DEBUG_WP("\n");
      TRACE_DEBUG("");
    } else {
      TRACE_DEBUG_WP("  ");
    }
  }
  TRACE_DEBUG_WP("FLASH modified blocks: %i  free blocks: %i/%i\n\n",flash_modified(), flash_free(), (uint16_t)(FLASH_BLOCKS_PER_PAGE * FLASH_EEPROM_NB_PAGES - 1));
}


void flash_init(void) {
  uint64_t* f = (uint64_t*)FLASH_EEPROM_ADDRESS;
  flashBlock_t* block;

  memset(eeprom, 0xff, sizeof(flashBlock_t) * FLASH_EEPROM_NB_BLOCKS);

  if (*f != FLASH_INIT_DOUBLEWORD) {
    flash_erase();
    TRACE_DEBUG("EEPROM FLASH erase \n");
  } else {
    for (uint8_t idx = 0; idx < FLASH_EEPROM_NB_BLOCKS; idx++) {
      block = flash_seek(idx);
      if (block) {
        TRACE_DEBUG("EEPROM load block %i\n", idx);
        memcpy(&eeprom[idx] ,block, sizeof(flashBlock_t));
      }
    }
  }

  eeprom_dump();

}
