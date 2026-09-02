/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * vtx_power_levels.h — canonical VTX power-level table shape.
 *
 * INDEXING: 1-based, matching Betaflight's native power field and
 * vtx_msp.c's powerTable[] convention -- valid levels are
 * 1..g_vtx_power_level_count inclusive. Index 0 exists in memory (arrays
 * are sized count+1) but is an unused placeholder -- do not read/write
 * it. This avoids a +1/-1 translation at every call site.
 *
 * STORAGE: on a USE_PA board, g_vtx_power_levels[] is a RUNTIME array in
 * RAM, not a target-fixed const. Each target's *_power.c defines the
 * compile-time DEFAULTS (g_vtx_power_level_defaults[]); at boot,
 * vtx_power_levels_init() copies those into g_vtx_power_levels[], then
 * overlays any calibration[]/detector[] values found in EEPROM (see
 * rf_pa_read_eeprom() in rf_pa.c) -- calibration values persist across
 * resets once written, target defaults are just the fallback when
 * EEPROM has nothing for a given level yet. mW/rtc6705_level/
 * ext_pa_enable are NOT EEPROM-overridden -- those are hardware facts
 * fixed by the target, not calibration data.
 *
 * On a no-PA board, there's nothing to calibrate -- g_vtx_power_levels[]
 * stays a plain target-fixed const, same as before.
 */
#ifndef VTX_POWER_LEVELS_H
#define VTX_POWER_LEVELS_H
#include <stdint.h>
#include <stdbool.h>
#include "rtc6705.h"

#define VTX_CAL_FREQ_POINTS 7

/* Compile-time upper bound on levels any target can define (sizes the
 * RAM array on USE_PA boards -- see vtx_power_levels.c). Actual level
 * count per target is g_vtx_power_level_count, always <= this. */
#define VTX_POWER_LEVEL_MAX 16

typedef struct {
    uint16_t        mW;             // advertised to the FC, informational only
    rtc6705_power_t rtc6705_level;  // RTC6705 register step -- always meaningful, RTC6705 is always present under BUILD_VARIANT_VTX
#if defined(USE_PA)
    bool     ext_pa_enable;                     // does this level engage the external boost PA stage (e.g. RTC76401)?
    uint16_t calibration[VTX_CAL_FREQ_POINTS];   // DAC mV per freq breakpoint (open-loop / PID setpoint)
    uint16_t detector[VTX_CAL_FREQ_POINTS];      // target VDET voltage in mV per freq breakpoint; 0 = open loop for this level
#endif
} vtx_power_level_t;

#if defined(USE_PA)
extern vtx_power_level_t g_vtx_power_levels[]; // RAM, mutable, sized VTX_POWER_LEVEL_MAX+1 -- see vtx_power_levels.c
void vtx_power_levels_init(void);              // copies target defaults in, then overlays EEPROM data. Call once at boot, after flash_init().
#else
extern const vtx_power_level_t g_vtx_power_levels[]; // no PA -> nothing to calibrate, stays target-fixed
#endif

extern const uint8_t g_vtx_power_level_count; // number of real levels; index 0 is an unused placeholder, valid range is 1..=this

#endif //VTX_POWER_LEVELS_H
