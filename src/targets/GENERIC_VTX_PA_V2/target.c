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
 * 1-based indexing (see vtx_power_levels.h) -- index 0 is an unused
 * placeholder whose calibration[] carries the frequency breakpoint list
 * (MHz), read by rf_pa.c and advertised via
 * vtx_msp_push_calibration_table(). This is the DEFAULTS table
 * (g_vtx_power_level_defaults[]), copied into the RAM-backed
 * g_vtx_power_levels[] at boot by vtx_power_levels_init() and then
 * overlaid with any EEPROM calibration data -- edit this file to change
 * what a freshly-flashed / never-calibrated board starts with, not what
 * an already-calibrated one uses.
 */
#include "main.h"
#include "vtx_power_levels.h"

const vtx_power_level_t g_vtx_power_level_defaults[] = {
    { 0,   0,                false, {5658,5695,5760,5800,5840,5905,5945}, {0,0,0,0,0,0,0} }, // index 0: NOT a real level -- calibration[] here is the frequency breakpoint list itself (MHz)
    { 1,   RTC6705_PA_3dBm,  false, {0,0,0,0,0,0,0}, {0,0,0,0,0,0,0} },                      // RTC6705 alone, PA disabled
    { 2,   RTC6705_PA_7dBm,  false, {0,0,0,0,0,0,0}, {0,0,0,0,0,0,0} },                      // RTC6705 alone, PA disabled
    { 5,   RTC6705_PA_3dBm,  true,  {0,0,0,0,0,0,0}, {100,100,100,100,100,100,100} },        // PA enabled, closed loop to VDET target
    { 10,  RTC6705_PA_3dBm,  true,  {0,0,0,0,0,0,0}, {100,100,100,100,100,100,100} },        // PA enabled, closed loop to VDET target
    { 25,  RTC6705_PA_7dBm,  true,  {0,0,0,0,0,0,0}, {100,100,100,100,100,100,100} },        // PA enabled, closed loop to VDET target
    { 50,  RTC6705_PA_7dBm,  true,  {0,0,0,0,0,0,0}, {100,100,100,100,100,100,100} },        // PA enabled, closed loop to VDET target
    { 100, RTC6705_PA_7dBm,  true,  {0,0,0,0,0,0,0}, {100,100,100,100,100,100,100} },        // PA enabled, closed loop to VDET target
    { 200, RTC6705_PA_7dBm,  true,  {0,0,0,0,0,0,0}, {100,100,100,100,100,100,100} },        // PA enabled, closed loop to VDET target
};

const uint8_t g_vtx_power_level_count = (sizeof(g_vtx_power_level_defaults) / sizeof(g_vtx_power_level_defaults[0])) - 1;
