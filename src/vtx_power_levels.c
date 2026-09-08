/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * vtx_power_levels.c — RAM-backed power table + frequency-breakpoint
 * table, with EEPROM persistence, for USE_PA boards only (a no-PA board
 * has nothing to calibrate, and keeps its target's g_vtx_power_levels[]
 * a plain const -- see vtx_power_levels.h and targets/GENERIC_VTX/target.c).
 *
 * EEPROM block layout (see flash.h's flashBlock_t: {idx, value[7]}).
 * Every block index is assigned directly from a level/field position --
 * no compaction is needed; FLASH_EEPROM_NB_BLOCKS (flash.c) has ample
 * headroom for every level (up to VTX_POWER_LEVEL_MAX) to get its own
 * fixed group:
 *   block 0             : header {schema_version, power_level_count, freq_point_count}
 *   blocks 1..3         : g_vtx_cal_frequencies_mhz[VTX_CAL_FREQ_POINTS_MAX] (9x u16 = 18 bytes)
 *   blocks EEPROM_LEVEL_BASE(L)..+5, for L = 1..VTX_POWER_LEVEL_MAX:
 *       level L's {mW(2B), ext_pa_enable(1B), rtc6705_level(1B),
 *       calibration[9](18B), detector[9](18B)} = 40 bytes, packed
 *       tightly (not field-aligned) across 6 blocks (42-byte capacity).
 *
 * A schema_version mismatch (or no header at all -- a fresh/erased chip)
 * means "don't trust these bytes", not "try to read them anyway" --
 * vtx_power_levels_init() resets everything to target.c's defaults in
 * that case. See VTX_POWER_TABLE_SCHEMA_VERSION's doc comment.
 */
#include "main.h"
#include "vtx_power_levels.h"

#if defined(USE_PA)
#include "flash.h"
#include <string.h>

#define EEPROM_HEADER_BLOCK        0
#define EEPROM_FREQ_TABLE_BLOCK0   1
#define EEPROM_FREQ_TABLE_BLOCKS   3
#define EEPROM_LEVEL_BLOCKS        6
#define EEPROM_LEVEL_BASE(level)   (EEPROM_FREQ_TABLE_BLOCK0 + EEPROM_FREQ_TABLE_BLOCKS + ((level) - 1) * EEPROM_LEVEL_BLOCKS)

vtx_power_level_t g_vtx_power_levels[VTX_POWER_LEVEL_MAX];
uint8_t g_vtx_power_level_count;
uint8_t g_vtx_cal_freq_point_count;
uint16_t g_vtx_cal_frequencies_mhz[VTX_CAL_FREQ_POINTS_MAX];

static void eeprom_read_blocks(uint8_t base, uint8_t nb_blocks, uint8_t *buf)
{
    memset(buf, 0, (size_t)nb_blocks * 7);
    for (uint8_t i = 0; i < nb_blocks; i++) {
        flashBlock_t block;
        block.idx = base + i;
        if (eeprom_read(&block)) {
            memcpy(&buf[i * 7], block.value, 7);
        }
    }
}

static void eeprom_write_blocks(uint8_t base, uint8_t nb_blocks, const uint8_t *buf)
{
    for (uint8_t i = 0; i < nb_blocks; i++) {
        flashBlock_t block;
        block.idx = base + i;
        memcpy(block.value, &buf[i * 7], 7);
        eeprom_write(&block);
    }
}

static void level_pack(const vtx_power_level_t *lvl, uint8_t *buf)
{
    buf[0] = (uint8_t)(lvl->mW & 0xFF);
    buf[1] = (uint8_t)((lvl->mW >> 8) & 0xFF);
    buf[2] = lvl->ext_pa_enable ? 1 : 0;
    buf[3] = (uint8_t)lvl->rtc6705_level;

    uint8_t off = 4;
    for (uint8_t c = 0; c < VTX_CAL_FREQ_POINTS_MAX; c++) {
        buf[off++] = (uint8_t)(lvl->calibration[c] & 0xFF);
        buf[off++] = (uint8_t)((lvl->calibration[c] >> 8) & 0xFF);
    }
    for (uint8_t c = 0; c < VTX_CAL_FREQ_POINTS_MAX; c++) {
        buf[off++] = (uint8_t)(lvl->detector[c] & 0xFF);
        buf[off++] = (uint8_t)((lvl->detector[c] >> 8) & 0xFF);
    }
}

static void level_unpack(vtx_power_level_t *lvl, const uint8_t *buf)
{
    lvl->mW = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    lvl->ext_pa_enable = buf[2] != 0;
    lvl->rtc6705_level = (rtc6705_power_t)buf[3];

    uint8_t off = 4;
    for (uint8_t c = 0; c < VTX_CAL_FREQ_POINTS_MAX; c++) {
        lvl->calibration[c] = (uint16_t)buf[off] | ((uint16_t)buf[off + 1] << 8);
        off += 2;
    }
    for (uint8_t c = 0; c < VTX_CAL_FREQ_POINTS_MAX; c++) {
        lvl->detector[c] = (uint16_t)buf[off] | ((uint16_t)buf[off + 1] << 8);
        off += 2;
    }
}

/* Stages level `level` into the EEPROM RAM shadow (flash.c's eeprom[]) --
 * does NOT call eeprom_save(). Actual persistence is deferred to
 * vtx_power_levels_flush_if_dirty(), called from the main loop -- see
 * that function's own doc comment for why this must never run inside an
 * MSP request/response turnaround. */
static void stage_level_to_eeprom(uint8_t level)
{
    if (!level || level > VTX_POWER_LEVEL_MAX) return;

    uint8_t buf[EEPROM_LEVEL_BLOCKS * 7];
    level_pack(&g_vtx_power_levels[level - 1], buf);
    eeprom_write_blocks(EEPROM_LEVEL_BASE(level), EEPROM_LEVEL_BLOCKS, buf);
}

static bool g_power_table_dirty;

bool vtx_power_levels_is_dirty(void)
{
    return g_power_table_dirty;
}

void vtx_power_levels_write_eeprom(uint8_t level)
{
    stage_level_to_eeprom(level);
    g_power_table_dirty = true;
}

static bool read_header(uint8_t *version, uint8_t *power_level_count, uint8_t *freq_point_count)
{
    flashBlock_t block;
    block.idx = EEPROM_HEADER_BLOCK;
    if (!eeprom_read(&block)) return false;

    *version = block.value[0];
    *power_level_count = block.value[1];
    *freq_point_count = block.value[2];
    return true;
}

static void write_header(void)
{
    flashBlock_t block = {0};
    block.idx = EEPROM_HEADER_BLOCK;
    block.value[0] = VTX_POWER_TABLE_SCHEMA_VERSION;
    block.value[1] = g_vtx_power_level_count;
    block.value[2] = g_vtx_cal_freq_point_count;
    eeprom_write(&block);
}

static void write_freq_table(void)
{
    uint8_t buf[EEPROM_FREQ_TABLE_BLOCKS * 7] = {0};
    for (uint8_t c = 0; c < VTX_CAL_FREQ_POINTS_MAX; c++) {
        buf[c * 2]     = (uint8_t)(g_vtx_cal_frequencies_mhz[c] & 0xFF);
        buf[c * 2 + 1] = (uint8_t)((g_vtx_cal_frequencies_mhz[c] >> 8) & 0xFF);
    }
    eeprom_write_blocks(EEPROM_FREQ_TABLE_BLOCK0, EEPROM_FREQ_TABLE_BLOCKS, buf);
}

static void read_freq_table(void)
{
    uint8_t buf[EEPROM_FREQ_TABLE_BLOCKS * 7];
    eeprom_read_blocks(EEPROM_FREQ_TABLE_BLOCK0, EEPROM_FREQ_TABLE_BLOCKS, buf);
    for (uint8_t c = 0; c < VTX_CAL_FREQ_POINTS_MAX; c++) {
        g_vtx_cal_frequencies_mhz[c] = (uint16_t)buf[c * 2] | ((uint16_t)buf[c * 2 + 1] << 8);
    }
}

/* Copies target.c's compiled-in defaults for `level` -- or, if `level` is
 * beyond what the target actually defines (a level the user grew the
 * table into, past g_vtx_power_level_default_count), the highest default
 * level's hardware mapping/calibration/detector: a safe known-good
 * starting point, with mW forced to 0 to flag "uncalibrated". */
static void seed_level_from_defaults(uint8_t level)
{
    uint8_t source = (level <= g_vtx_power_level_default_count) ? level : g_vtx_power_level_default_count;
    g_vtx_power_levels[level - 1] = g_vtx_power_level_defaults[source - 1];
    if (source != level) {
        g_vtx_power_levels[level - 1].mW = 0;
    }
}

static void reset_all_to_defaults_in_ram(uint8_t power_level_count, uint8_t freq_point_count, const uint16_t *frequencies_mhz)
{
    g_vtx_power_level_count = power_level_count;
    g_vtx_cal_freq_point_count = freq_point_count;

    memset(g_vtx_cal_frequencies_mhz, 0, sizeof(g_vtx_cal_frequencies_mhz));
    memcpy(g_vtx_cal_frequencies_mhz, frequencies_mhz, (size_t)freq_point_count * sizeof(uint16_t));

    for (uint8_t level = 1; level <= power_level_count; level++) {
        seed_level_from_defaults(level);
    }
}

void vtx_power_levels_reset_to_defaults(void)
{
    reset_all_to_defaults_in_ram(g_vtx_power_level_default_count, g_vtx_cal_freq_default_count, g_vtx_cal_freq_defaults_mhz);
    g_power_table_dirty = true;
}

/* Called once per main-loop iteration (see main.c) -- never from an MSP
 * handler. eeprom_save()/flash_push()/flash_seek() (flash.c) are full
 * linear scans over every EEPROM block ever used this session, slow
 * enough on real hardware to blow well past any request/response
 * turnaround, and that only gets worse the more levels have been
 * touched. Deferring it here means SET_PACALTABLE/SET_VTX_POWER_TABLE_LAYOUT/
 * a factory reset all return immediately (RAM already reflects the new
 * state) and this does the slow part in the background -- a calibration
 * tool polls vtx_power_levels_is_dirty() (exposed over MSP) instead of
 * guessing how long that might take. */
void vtx_power_levels_flush_if_dirty(void)
{
    if (!g_power_table_dirty) return;

    write_header();
    write_freq_table();
    for (uint8_t level = 1; level <= g_vtx_power_level_count; level++) {
        stage_level_to_eeprom(level);
    }
    eeprom_save();

    g_power_table_dirty = false;
}

static bool frequencies_are_valid(uint8_t freq_point_count, const uint16_t *frequencies_mhz)
{
    if (freq_point_count < 2 || freq_point_count > VTX_CAL_FREQ_POINTS_MAX) return false;

    for (uint8_t i = 0; i < freq_point_count; i++) {
        if (frequencies_mhz[i] < 5600 || frequencies_mhz[i] > 6000) return false;
        if (i > 0 && frequencies_mhz[i] <= frequencies_mhz[i - 1]) return false; // strictly ascending -- rf_pa.c's breakpoint search assumes this
    }
    return true;
}

bool vtx_power_levels_set_layout(uint8_t power_level_count, uint8_t freq_point_count, const uint16_t *frequencies_mhz)
{
    if (power_level_count < 1 || power_level_count > VTX_POWER_LEVEL_MAX) {
        TRACE_ERROR("Rejected power table layout, bad level count. value: %u\n", power_level_count);
        return false;
    }
    if (!frequencies_are_valid(freq_point_count, frequencies_mhz)) {
        TRACE_ERROR("Rejected power table layout, bad frequency table. count: %u\n", freq_point_count);
        return false;
    }

    /* Changing the frequency breakpoints invalidates every level's
     * calibration[]/detector[] -- interpolating old calibration data
     * against a different breakpoint set would be actively wrong (the
     * values were only ever validated at the OLD frequencies). Simplest
     * safe option: every level goes back to its target default (or the
     * nearest one, per seed_level_from_defaults()) and the user
     * recalibrates. */
    reset_all_to_defaults_in_ram(power_level_count, freq_point_count, frequencies_mhz);
    g_power_table_dirty = true;

    TRACE_INFO("Power table layout set. levels: %u, freq points: %u\n", power_level_count, freq_point_count);
    return true;
}

void vtx_power_levels_init(void)
{
    uint8_t version = 0, power_level_count = 0, freq_point_count = 0;
    bool have_header = read_header(&version, &power_level_count, &freq_point_count);

    bool header_is_sane = have_header
        && version == VTX_POWER_TABLE_SCHEMA_VERSION
        && power_level_count >= 1 && power_level_count <= VTX_POWER_LEVEL_MAX
        && freq_point_count >= 2 && freq_point_count <= VTX_CAL_FREQ_POINTS_MAX;

    if (!header_is_sane) {
        TRACE_INFO("EEPROM power table absent/stale, resetting to target defaults\n");
        vtx_power_levels_reset_to_defaults();
        return;
    }

    g_vtx_power_level_count = power_level_count;
    g_vtx_cal_freq_point_count = freq_point_count;
    read_freq_table();

    for (uint8_t level = 1; level <= power_level_count; level++) {
        uint8_t buf[EEPROM_LEVEL_BLOCKS * 7];
        eeprom_read_blocks(EEPROM_LEVEL_BASE(level), EEPROM_LEVEL_BLOCKS, buf);
        level_unpack(&g_vtx_power_levels[level - 1], buf);
    }
}

#endif //USE_PA
