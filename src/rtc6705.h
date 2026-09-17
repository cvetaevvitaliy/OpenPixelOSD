/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef RTC6705_H
#define RTC6705_H
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  RTC6705_PA_3dBm  = 0,  // PA5G_PW=00
  RTC6705_PA_7dBm  = 1,  // PA5G_PW=01
  RTC6705_PA_11dBm = 2,  // PA5G_PW=10
  RTC6705_PA_13dBm = 3   // PA5G_PW=11
} rtc6705_power_t;

bool rtc6705_init(void);
void rtc6705_allow_power_writes(bool allow);
void rtc6705_set_power(rtc6705_power_t level);
rtc6705_power_t rtc6705_get_power(void);
uint32_t rtc6705_set_frequency(uint32_t freq_mhz);

#endif //RTC6705_H
