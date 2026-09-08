/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * vtx_power_levels.h — canonical VTX power-level table shape.
 *
 * INDEXING: level NUMBERS are 1-based, matching Betaflight's native power
 * field and the MSP wire protocol (SET_VTX_CONFIG's power byte,
 * PACALTABLE's idx byte, etc) -- valid levels are 1..g_vtx_power_level_count
 * inclusive. The underlying ARRAY is plain 0-based and exactly
 * g_vtx_power_level_count (or VTX_POWER_LEVEL_MAX) entries long -- level L
 * lives at g_vtx_power_levels[L - 1]. Every call site does that -1 itself;
 * there is no reserved/unused slot 0 in the array.
 *
 * STORAGE: on a USE_PA board, g_vtx_power_levels[] is a RUNTIME array in
 * RAM, not a target-fixed const. Each target's target.c defines the
 * compile-time DEFAULTS (g_vtx_power_level_defaults[]); at boot,
 * vtx_power_levels_init() copies those into g_vtx_power_levels[], then
 * overlays whatever is in EEPROM (see vtx_power_levels.c) -- EEPROM data
 * persists across resets once written, target defaults are just the
 * fallback for a freshly-flashed/never-configured board.
 *
 * RUNTIME CONFIGURABILITY: g_vtx_power_level_count (how many of
 * g_vtx_power_levels[] are real, 1..VTX_POWER_LEVEL_MAX) and
 * g_vtx_cal_freq_point_count / g_vtx_cal_frequencies_mhz[] (how many
 * calibration-frequency breakpoints, and their values, 1..
 * VTX_CAL_FREQ_POINTS_MAX) are both settable at runtime over MSP -- see
 * vtx_msp_set_power_table_layout() -- and persisted. mW/rtc6705_level/
 * ext_pa_enable/calibration[]/detector[] are likewise settable per level
 * (vtx_msp_set_calibration_table()), not fixed hardware facts as before.
 *
 * On a no-PA board, there's nothing to calibrate -- g_vtx_power_levels[]
 * stays a plain target-fixed const, same as before, and none of the
 * runtime-layout machinery below applies.
 */
#ifndef VTX_POWER_LEVELS_H
#define VTX_POWER_LEVELS_H
#include <stdint.h>
#include <stdbool.h>
#include "rtc6705.h"

/* Compile-time upper bound on calibration-frequency breakpoints any
 * target/user can configure. Matches the ImmersionRC power meter V2's own
 * fixed 5.6-6.0 GHz table size (5600,5650,...,6000 MHz -- 9 entries), so a
 * user can calibrate against every point that meter can measure. */
#define VTX_CAL_FREQ_POINTS_MAX 9

/* Compile-time upper bound on levels any target/user can define (sizes
 * the RAM array on USE_PA boards -- see vtx_power_levels.c). Actual level
 * count is g_vtx_power_level_count, always <= this. */
#define VTX_POWER_LEVEL_MAX 16

/* Bumped whenever the EEPROM block layout in vtx_power_levels.c changes
 * shape -- a mismatch at boot means "don't trust these bytes", not "try
 * to read them anyway" (old-layout bytes reinterpreted under the new
 * layout could feed the PA loop garbage DAC values). */
#define VTX_POWER_TABLE_SCHEMA_VERSION 1

typedef struct {
    uint16_t        mW;             // advertised to the FC, informational only
    rtc6705_power_t rtc6705_level;  // RTC6705 register step -- always meaningful, RTC6705 is always present under USE_VTX
#if defined(USE_PA)
    bool     ext_pa_enable;                        // does this level engage the external boost PA stage (e.g. RTC76401)?
    uint16_t calibration[VTX_CAL_FREQ_POINTS_MAX];  // DAC mV per freq breakpoint (open-loop / PID setpoint); only the first g_vtx_cal_freq_point_count entries are meaningful
    uint16_t detector[VTX_CAL_FREQ_POINTS_MAX];     // target VDET voltage in mV per freq breakpoint; 0 = open loop for this level
#endif
} vtx_power_level_t;

#if defined(USE_PA)
extern vtx_power_level_t g_vtx_power_levels[]; // RAM, mutable, sized VTX_POWER_LEVEL_MAX -- see vtx_power_levels.c
extern uint8_t g_vtx_power_level_count;        // RAM, mutable -- number of real levels currently active, 1..VTX_POWER_LEVEL_MAX
extern uint8_t g_vtx_cal_freq_point_count;     // RAM, mutable -- number of calibration-frequency breakpoints currently active, 1..VTX_CAL_FREQ_POINTS_MAX
extern uint16_t g_vtx_cal_frequencies_mhz[VTX_CAL_FREQ_POINTS_MAX]; // RAM, mutable -- the breakpoints themselves, ascending, only the first g_vtx_cal_freq_point_count meaningful

void vtx_power_levels_init(void); // loads EEPROM if its schema matches, else resets everything to target defaults. Call once at boot, after flash_init().

/* Validates and applies a new layout (level count + frequency
 * breakpoints) to RAM, and resets every level's calibration[]/detector[]
 * back to its target default (or the nearest one, for a level beyond the
 * target's own default count) -- see vtx_power_levels.c's doc comment on
 * why a changed frequency table can't keep old calibration data. Returns
 * false (no state changed) if the request is out of bounds or malformed.
 * Does NOT block on EEPROM -- marks the table dirty and returns
 * immediately; see vtx_power_levels_flush_if_dirty(). */
bool vtx_power_levels_set_layout(uint8_t power_level_count, uint8_t freq_point_count, const uint16_t *frequencies_mhz);

/* Stages level `level`'s current mW/ext_pa_enable/rtc6705_level/
 * calibration[]/detector[] for persistence and marks the table dirty.
 * No-op if level is out of range. Does NOT block on EEPROM -- see
 * vtx_power_levels_flush_if_dirty(). */
void vtx_power_levels_write_eeprom(uint8_t level);

/* Resets every level (1..g_vtx_power_level_count) and the layout itself
 * back to this target's compiled-in defaults, in RAM, and marks the
 * table dirty. Does NOT block on EEPROM -- see
 * vtx_power_levels_flush_if_dirty(). */
void vtx_power_levels_reset_to_defaults(void);

/* True from the moment a layout/level change is accepted until the
 * pending EEPROM write actually lands -- see vtx_power_levels_flush_if_dirty().
 * Exposed over MSP (MSP_VTX_POWER_TABLE_LAYOUT) so a calibration tool can
 * poll for "write finished" instead of guessing how long flash
 * housekeeping might take. */
bool vtx_power_levels_is_dirty(void);

/* Call once per main-loop iteration (never from an MSP handler -- the
 * whole point is that SET_PACALTABLE/SET_VTX_POWER_TABLE_LAYOUT/factory
 * reset return immediately and this does the slow part afterward).
 * No-op unless vtx_power_levels_is_dirty(). eeprom_save() (see flash.c)
 * is a set of full linear scans over every EEPROM block ever used this
 * session, so this can take a while -- that's exactly why it must never
 * run inside a request/response turnaround. */
void vtx_power_levels_flush_if_dirty(void);

/* Provided by the active target's target.c -- e.g.
 * targets/GENERIC_VTX_PA_RTC76401/target.c. Sign-correct, target-specific
 * "safe/uncalibrated" values -- NOT a generic zero, which would be
 * actively dangerous on an inverted-PA_DAC_SIGN board (low DAC = high
 * output there). This is what a factory reset copies from, specifically
 * so that operation can never depend on a value the tool itself made up. */
extern const vtx_power_level_t g_vtx_power_level_defaults[];
extern const uint8_t g_vtx_power_level_default_count; // number of defaults target.c actually defines; may be < VTX_POWER_LEVEL_MAX

/* Default calibration-frequency breakpoints (MHz, ascending), provided by
 * the active target's target.c alongside g_vtx_power_level_defaults[]. */
extern const uint16_t g_vtx_cal_freq_defaults_mhz[];
extern const uint8_t g_vtx_cal_freq_default_count; // <= VTX_CAL_FREQ_POINTS_MAX
#else
extern const vtx_power_level_t g_vtx_power_levels[]; // no PA -> nothing to calibrate, stays target-fixed. 0-based array, level L at [L-1]
extern const uint8_t g_vtx_power_level_count;         // number of real levels, valid range is 1..=this
#endif

#endif //VTX_POWER_LEVELS_H
