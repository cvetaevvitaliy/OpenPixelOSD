/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "flash.h"
#include "main.h"
#include "video_overlay.h"
#include "vtx_msp.h"
#include "settings.h"

#include <stdbool.h>
#include <string.h>

#define BLOCK_SETTINGS      31

setting_t settings;

void settings_load(void) {
  #if defined(USE_VTX)
  vtx_config_t *vtx_config = (vtx_config_t*)vtx_get_config();
  #endif

  settings.idx = BLOCK_SETTINGS;
  if(!eeprom_read((flashBlock_t*) &settings)) {
    settings.camswitchEnabled = false;
    settings.displayportEnabled = true;
    settings.band = 4;
    settings.channel = 4;
    settings.power = 1;
    settings.frequency = 5800;
  } 

  #if defined(USE_VTX)
  vtx_config->band = settings.band;
  vtx_config->channel = settings.channel;
  vtx_config->power = settings.power;
  vtx_config->frequency = settings.frequency;
  #endif
}

void settings_save(void) {
  #if defined(USE_VTX)
  vtx_config_t *vtx_config = (vtx_config_t*)vtx_get_config();

  settings.idx = BLOCK_SETTINGS;
  settings.band = vtx_config->band;
  settings.channel = vtx_config->channel;
  settings.power = vtx_config->power;
  settings.frequency = vtx_config->frequency;
  #endif

  eeprom_write((flashBlock_t*) &settings);
  eeprom_save();
}

