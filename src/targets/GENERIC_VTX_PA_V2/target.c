/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/GENERIC_VTX_PA_V2/target.c — power table for the generic
 * RTC6705 VTX board with a Skyworks SE5004L-class linear external PA, on
 * the optimal reference pinout.
 *
 * The PA has a real enable line (PA_ON_* / the PA's VREF input, see
 * target.h), so ext_pa_enable below is functional: rf_pa_boost_on()/off()
 * actually toggle the amplifier. Levels with ext_pa_enable=false run the
 * RTC6705 alone with the PA held disabled.
 *
 * Because the op-amp gate driver (see target.h) linearises the bias, the
 * boost-enabled levels run CLOSED LOOP: calibration[] is left at 0 (the
 * loop trims the DAC up from the bottom of its range) and detector[]
 * carries the target VDET voltage in mV per frequency breakpoint that
 * rf_pa.c's PID drives toward.
 *
 * PLACEHOLDER values -- the detector[] targets here are a single uniform
 * starting point, not per-level / per-frequency power-meter data. Sweep
 * each level against a real power meter and replace them before trusting
 * the mW labels.
 *
 * Level NUMBERS are 1-based (see vtx_power_levels.h) but this array is
 * plain 0-based -- level N is entry [N-1] below, there is no reserved
 * slot 0. This is the DEFAULTS table (g_vtx_power_level_defaults[]),
 * copied into the RAM-backed g_vtx_power_levels[] at boot by
 * vtx_power_levels_init() and then overlaid with any EEPROM data (see
 * vtx_power_levels.c) -- edit this file to change what a freshly-
 * flashed/never-configured board starts with, not what an
 * already-configured one uses. calibration[]/detector[] are
 * VTX_CAL_FREQ_POINTS_MAX-sized (this file's own compile-time ceiling on
 * breakpoints) -- only the first g_vtx_cal_freq_default_count entries
 * below are meaningful; the frequency breakpoints themselves live in
 * g_vtx_cal_freq_defaults_mhz[] below, not smuggled into this table.
 */
#include "main.h"
#include "vtx_power_levels.h"

const vtx_power_level_t g_vtx_power_level_defaults[] = {
    { 1,   RTC6705_PA_3dBm,  false, {0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0} },                      // level 1: RTC6705 alone, PA disabled
    { 2,   RTC6705_PA_7dBm,  false, {0,0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0,0} },                      // level 2: RTC6705 alone, PA disabled
    { 5,   RTC6705_PA_3dBm,  true,  {0,0,0,0,0,0,0,0,0}, {100,100,100,100,100,100,100,100,100} },        // level 3: PA enabled, closed loop to VDET target
    { 10,  RTC6705_PA_3dBm,  true,  {0,0,0,0,0,0,0,0,0}, {100,100,100,100,100,100,100,100,100} },        // level 4: PA enabled, closed loop to VDET target
    { 25,  RTC6705_PA_7dBm,  true,  {0,0,0,0,0,0,0,0,0}, {100,100,100,100,100,100,100,100,100} },        // level 5: PA enabled, closed loop to VDET target
    { 50,  RTC6705_PA_7dBm,  true,  {0,0,0,0,0,0,0,0,0}, {100,100,100,100,100,100,100,100,100} },        // level 6: PA enabled, closed loop to VDET target
    { 100, RTC6705_PA_7dBm,  true,  {0,0,0,0,0,0,0,0,0}, {100,100,100,100,100,100,100,100,100} },        // level 7: PA enabled, closed loop to VDET target
    { 200, RTC6705_PA_7dBm,  true,  {0,0,0,0,0,0,0,0,0}, {100,100,100,100,100,100,100,100,100} },        // level 8: PA enabled, closed loop to VDET target
};

const uint8_t g_vtx_power_level_default_count = sizeof(g_vtx_power_level_defaults) / sizeof(g_vtx_power_level_defaults[0]);

const uint16_t g_vtx_cal_freq_defaults_mhz[] = {5658,5695,5760,5800,5840,5905,5945};
const uint8_t g_vtx_cal_freq_default_count = sizeof(g_vtx_cal_freq_defaults_mhz) / sizeof(g_vtx_cal_freq_defaults_mhz[0]);
