/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef RF_PA_H
#define RF_PA_H

#if defined(USE_PA)

#include <stdint.h>
#include <stdbool.h>
#include "vtx_power_levels.h"

void rf_pa_init(void);
void rf_pa_apply_level(const vtx_power_level_t *lvl);
void rf_pa_disable(void);
void rf_pa_restore(void);

uint16_t rf_pa_read_vdet_mv(void);
uint16_t rf_pa_get_vref_mv(void);
void rf_pa_set_vref_mv(uint16_t mv);

float rf_pa_read_ntc_temp_c(void);
uint16_t rf_pa_read_ntc_raw(void);
#if defined(ADC_NTC_INSTANCE)

float rf_pa_ntc_raw_to_celsius(uint16_t adc_raw);
#endif

void rf_pa_set_calibration(uint16_t mv);

void rf_pa_calibration_session_begin(void);
void rf_pa_calibration_session_end(void);
bool rf_pa_calibration_session_is_active(void);

void rf_pa_manual_boost_set(bool on);
void rf_pa_manual_boost_clear(void);
bool rf_pa_manual_boost_override_active(void);

double rf_pa_get_detector_mv(void);

bool rf_pa_boost_is_on(void);
bool rf_pa_pid_active(void);
bool rf_pa_calibration_override_active(void);

void rf_pa_read_eeprom(uint8_t level);
void rf_pa_write_eeprom(uint8_t level);

/* Call periodically (e.g. every main-loop iteration) to run the DAC bias
 * PID loop against the active level's detector target, when it has one. */
void rf_pa_loop(bool field_edge_flag);

#endif //USE_PA
#endif //RF_PA_H
