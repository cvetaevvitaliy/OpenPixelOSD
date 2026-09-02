/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/GENERIC_VTX/target.c — power table for a board with RTC6705 but no
 * PA feature (USE_PA undefined here). Just RTC6705's own 4 register
 * steps -- no boost stage, no DAC/detector fields (they don't exist in
 * vtx_power_level_t on this build).
 *
 * 1-based indexing (see vtx_power_levels.h) -- index 0 is an unused
 * placeholder. No EEPROM overlay on this board (nothing to calibrate),
 * so this stays a plain target-fixed const, unlike the USE_PA targets.
 *
 * mW figures are rough/unverified -- RTC6705's own output varies by
 * board layout and matching network. Treat as a starting point, not a
 * calibrated fact.
 */
#include "main.h"
#include "vtx_power_levels.h"

const vtx_power_level_t g_vtx_power_levels[] = {
    { 0,  0                 }, // index 0: unused placeholder
    { 2,  RTC6705_PA_3dBm   },
    { 5,  RTC6705_PA_7dBm   },
    { 12, RTC6705_PA_11dBm  },
    { 20, RTC6705_PA_13dBm  },
};

const uint8_t g_vtx_power_level_count = (sizeof(g_vtx_power_levels) / sizeof(g_vtx_power_levels[0])) - 1;
