/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/GENERIC_VTX_PA/target.c — power table for the baseline PA: a
 * single DAC-biased amplifier, no separate boost-enable GPIO (this
 * target defines no PA_ON_Pin, so rf_pa_boost_on()/off() are no-ops and
 * ext_pa_enable's value here doesn't matter functionally -- set true for
 * clarity).
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
    { 25,  RTC6705_PA_7dBm,  true,  {800,800,800,800,800,800,800,0,0},         {0,0,0,0,0,0,0,0,0} },
    { 100, RTC6705_PA_11dBm, true,  {1400,1400,1400,1400,1400,1400,1400,1400,1400}, {0,0,0,0,0,0,0,0,0} },
    { 200, RTC6705_PA_13dBm, true,  {1800,1800,1800,1800,1800,1800,1800,1800,1800}, {0,0,0,0,0,0,0,0,0} },
    { 800, RTC6705_PA_13dBm, true,  {2400,2400,2400,2400,2400,2400,2400,2400,2400}, {0,0,0,0,0,0,0,0,0} },
};

const uint8_t g_vtx_power_level_default_count = sizeof(g_vtx_power_level_defaults) / sizeof(g_vtx_power_level_defaults[0]);

const uint16_t g_vtx_cal_freq_defaults_mhz[] = {5658,5695,5760,5800,5840,5905,5945};
const uint8_t g_vtx_cal_freq_default_count = sizeof(g_vtx_cal_freq_defaults_mhz) / sizeof(g_vtx_cal_freq_defaults_mhz[0]);
