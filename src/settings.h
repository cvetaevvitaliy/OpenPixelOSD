#pragma once

#include <stdbool.h>

typedef struct {
  uint8_t idx;
  bool displayportEnabled : 1;
  bool camswitchEnabled : 1;
  bool activeVideoInput : 1;
  uint8_t band;
  uint8_t channel;
  uint8_t power;
  uint8_t spare;
  uint16_t frequency;
} setting_t;

extern setting_t settings;

void settings_load(void);
void settings_save(void);
