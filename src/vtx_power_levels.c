/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * vtx_power_levels.c — RAM-backed power table + EEPROM persistence, for
 * USE_PA boards only (a no-PA board has nothing to calibrate, and keeps
 * its target's g_vtx_power_levels[] a plain const -- see
 * vtx_power_levels.h and targets/generic_power.c).
 *
 * EEPROM block layout (see flash.h's flashBlock_t: {idx, value[7]}, 32
 * blocks total, block 31 already claimed by settings.c -- see
 * settings.c): 4 blocks per CALIBRATED level (ext_pa_enable==true only;
 * the RTC6705-alone levels are simple/structural and were never
 * power-meter calibrated in the first place, so persisting them wastes
 * scarce block budget for no benefit). Blocks are assigned by a compact
 * 0-based "calibration slot" -- the Nth calibrated level encountered in
 * array order, NOT the raw level number -- so block usage stays tight
 * regardless of how levels are numbered/spaced:
 *   slot N -> blocks [4N, 4N+1, 4N+2, 4N+3]
 *           = calibration[0..3], calibration[4..6]+pad,
 *             detector[0..3],    detector[4..6]+pad
 * With 31 blocks free (32 total minus 1 for settings), that's room for
 * 7 calibrated levels. The RTC76401 target currently has 4
 * (50/100/150/200mW) -- comfortable margin. Check this math again
 * before adding more calibrated levels to any target.
 */
#include "main.h"
#include "vtx_power_levels.h"

#if defined(USE_PA)
#include "flash.h"
#include <string.h>

#define EEPROM_BLOCKS_PER_LEVEL 4

/* Provided by the active target's *_power.c -- e.g.
 * targets/generic_vtx_pa_rtc76401_power.c. */
extern const vtx_power_level_t g_vtx_power_level_defaults[];

vtx_power_level_t g_vtx_power_levels[VTX_POWER_LEVEL_MAX + 1];

static uint8_t calibration_slot_for_level(uint8_t level)
{
    uint8_t slot = 0;
    for (uint8_t i = 1; i < level; i++) {
        if (i <= g_vtx_power_level_count && g_vtx_power_level_defaults[i].ext_pa_enable) {
            slot++;
        }
    }
    return slot;
}

void rf_pa_read_eeprom(uint8_t level)
{
    if (!level || level > g_vtx_power_level_count) return;
    if (!g_vtx_power_level_defaults[level].ext_pa_enable) return; // not persisted -- keeps its compiled-in default

    uint8_t base = calibration_slot_for_level(level) * EEPROM_BLOCKS_PER_LEVEL;
    flashBlock_t block;
    uint8_t *cal_bytes = (uint8_t *)g_vtx_power_levels[level].calibration;
    uint8_t *det_bytes = (uint8_t *)g_vtx_power_levels[level].detector;

    block.idx = base + 0;
    if (eeprom_read(&block)) memcpy(&cal_bytes[0], block.value, 7);
    block.idx = base + 1;
    if (eeprom_read(&block)) memcpy(&cal_bytes[7], block.value, 7);
    block.idx = base + 2;
    if (eeprom_read(&block)) memcpy(&det_bytes[0], block.value, 7);
    block.idx = base + 3;
    if (eeprom_read(&block)) memcpy(&det_bytes[7], block.value, 7);
}

void rf_pa_write_eeprom(uint8_t level)
{
    if (!level || level > g_vtx_power_level_count) return;
    if (!g_vtx_power_level_defaults[level].ext_pa_enable) return; // not persisted for this level

    uint8_t base = calibration_slot_for_level(level) * EEPROM_BLOCKS_PER_LEVEL;
    flashBlock_t block;
    uint8_t *cal_bytes = (uint8_t *)g_vtx_power_levels[level].calibration;
    uint8_t *det_bytes = (uint8_t *)g_vtx_power_levels[level].detector;

    block.idx = base + 0;
    memcpy(block.value, &cal_bytes[0], 7);
    eeprom_write(&block);
    block.idx = base + 1;
    memcpy(block.value, &cal_bytes[7], 7);
    eeprom_write(&block);
    block.idx = base + 2;
    memcpy(block.value, &det_bytes[0], 7);
    eeprom_write(&block);
    block.idx = base + 3;
    memcpy(block.value, &det_bytes[7], 7);
    eeprom_write(&block);

    eeprom_save();
}

void vtx_power_levels_init(void)
{
    memcpy(g_vtx_power_levels, g_vtx_power_level_defaults,
           sizeof(vtx_power_level_t) * (g_vtx_power_level_count + 1));

    for (uint8_t level = 1; level <= g_vtx_power_level_count; level++) {
        rf_pa_read_eeprom(level);
    }
}

#endif //USE_PA
