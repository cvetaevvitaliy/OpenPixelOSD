/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/GENERIC_VTX_PA/target.c — power table for the baseline PA
 * (PA_GENERIC): a single DAC-biased amplifier, no separate boost-enable
 * GPIO (rf_pa_boost_on()/off() are no-ops without PA_RTC76401, so
 * ext_pa_enable's value here doesn't matter functionally -- set true for
 * clarity).
 *
 * 1-based indexing (see vtx_power_levels.h) -- index 0 is an unused
 * placeholder. This is the DEFAULTS table (g_vtx_power_level_defaults[]),
 * copied into the RAM-backed g_vtx_power_levels[] at boot by
 * vtx_power_levels_init() and then overlaid with any EEPROM calibration
 * data -- edit this file to change what a freshly-flashed/never-
 * calibrated board starts with, not what an already-calibrated one uses.
 *
 * PLACEHOLDER calibration values -- this board's actual bias topology
 * was never fully characterized in this codebase (the original DAC-based
 * rf_pa.c approach was inherited from a different, unspecified PA design).
 * Do not trust these numbers without a real bench sweep against a power
 * meter, the same way the RTC76401 table required.
 */
#include "main.h"
#include "vtx_power_levels.h"

const vtx_power_level_t g_vtx_power_level_defaults[] = {
    { 0,   0,                false, {5658,5695,5760,5800,5840,5905,5945}, {0,0,0,0,0,0,0} }, // index 0: NOT a real level -- calibration[] here is the frequency breakpoint list itself (MHz), read by rf_pa.c and advertised via vtx_msp_push_calibration_table()
    { 25,  RTC6705_PA_7dBm,  true,  {800,800,800,800,800,800,800},         {0,0,0,0,0,0,0} },
    { 100, RTC6705_PA_11dBm, true,  {1400,1400,1400,1400,1400,1400,1400}, {0,0,0,0,0,0,0} },
    { 200, RTC6705_PA_13dBm, true,  {1800,1800,1800,1800,1800,1800,1800}, {0,0,0,0,0,0,0} },
    { 800, RTC6705_PA_13dBm, true,  {2400,2400,2400,2400,2400,2400,2400}, {0,0,0,0,0,0,0} },
};

const uint8_t g_vtx_power_level_count = (sizeof(g_vtx_power_level_defaults) / sizeof(g_vtx_power_level_defaults[0])) - 1;
