/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "vtx_msp.h"
#include "main.h"

#if defined(USE_VTX)

#include "msp.h"
#include "msp_displayport.h"
#include "msp_protocol.h"
#include "rf_pa.h"
#include "rtc6705.h"
#include "uart.h"
#include "usb.h"
#include "vtx_power_levels.h"
#include "flash.h"
#include "led.h" // ASSUMPTION: already present in your tree (not something added by this file) -- if not, either add it or strip the led_set() calls below

#include <string.h>
#include <stdio.h>

#ifndef BAND_TABLE
#define BAND_TABLE          BAND_TABLE_DEFAULT
#endif

static void vtx_apply_hw(const vtx_config_t *cfg);
static const vtx_band_t g_bands[] = BAND_TABLE;

#define NUM_BANDS           (sizeof(g_bands)/sizeof(g_bands[0]))

static vtx_config_t g_cfg = {
    .band = 5,
    .channel = 1,
    .frequency = 5800,
    .power = 1,
    .pitmode = 0,
    .configSet = 0,
};

const vtx_config_t* vtx_get_config(void)
{
    return &g_cfg;
}

const char* vtx_get_band_name(uint8_t band)
{
    return (char*)&g_bands[band].band_name;
}

uint8_t vtx_get_band_count(void)
{
    return NUM_BANDS;
}

uint16_t vtx_get_power_mw(void)
{
    return g_vtx_power_levels[g_cfg.power - 1].mW;
}

uint16_t vtx_get_frequency(uint8_t band, uint8_t channel)
{
    return g_bands[band].freq[channel];
}

void vtx_set_pitmode(uint8_t pitmode)
{
    g_cfg.pitmode = pitmode;
    vtx_apply_hw(&g_cfg);
}

static inline void msp_tx_send(uint8_t owner, const uint8_t *buf, uint16_t len)
{
    if (owner == MSP_OWNER_USB) {
        usb_uart_write_bytes((const char*)buf, len);
    } else if (owner == MSP_OWNER_UART) {
        uart1_tx_dma((uint8_t*)buf, len);
    }
}

static inline bool freq_is_in_58ghz(uint16_t mhz)
{
    return (mhz >= 5600 && mhz <= 6000);
}

void vtx_set_band_channel(int8_t band, uint8_t channel)
{
    if (freq_is_in_58ghz(g_bands[band].freq[channel])) {
        g_cfg.band = band;
        g_cfg.channel = channel;
        g_cfg.frequency = g_bands[band-1].freq[channel-1];
        vtx_apply_hw(&g_cfg);
    }
}

void vtx_set_power(int8_t power)
{
    g_cfg.power = power;
    vtx_apply_hw(&g_cfg);
}

static void vtx_apply_hw(const vtx_config_t *cfg)
{
    printf("vtx_apply_hw: band=%d ch=%d freq=%d power=%d pit=%d\n",
           cfg->band, cfg->channel, cfg->frequency, cfg->power, cfg->pitmode);

    /* Disable external RF Power Amplifier and set RTC6705 to its lowest
     * register step as a safe default -- matches rf_pa_apply_level(NULL)
     * + RTC6705_PA_3dBm, our equivalents of the reference's
     * rf_pa_set_power_level(RF_PA_PWR_OFF) / RTC6705_LOW. */
#if defined(USE_PA)
    rf_pa_apply_level(NULL);
#endif
    rtc6705_allow_power_writes(true);
    rtc6705_set_power(RTC6705_PA_3dBm);
    rtc6705_allow_power_writes(false);

    if (g_cfg.configSet)
        led_set(0, RGB_BLUE);
    else
        led_set(0, RGB_RED);

    /* Program synthesizer frequency (MHz) */
    if (freq_is_in_58ghz(cfg->frequency)) {
        rtc6705_set_frequency(cfg->frequency);

        if (!cfg->pitmode) {
            const vtx_power_level_t *lvl = &g_vtx_power_levels[cfg->power - 1];

            rtc6705_allow_power_writes(true);
            rtc6705_set_power(lvl->rtc6705_level);
            rtc6705_allow_power_writes(false);

#if defined(USE_PA)
            rf_pa_apply_level(lvl);
#endif
            led_set(0, RGB_GREEN);
        }
    } else {
        rtc6705_set_frequency(0);
        led_set(0, RGB_RED);
    }
}

static void handle_msp_set_vtx_config(uint8_t owner, const uint8_t *payload, uint16_t data_size)
{
    /* MSP_VTX_CONFIG payload (Betaflight): 15 bytes
       [0] vtxType
       [1] band (1..N)
       [2] channel (1..8)
       [3] power (1..P)  <-- BF is 1-based
       [4] pit mode (0/1)
       [5] freq LSB
       [6] freq MSB      <-- MHz (non-zero overrides band/channel)
       [7] deviceIsReady
       [8] lowPowerDisarm (0/1)
       [9]  pitModeFreq LSB
       [10] pitModeFreq MSB
       [11] vtxTableAvailable (0/1)
       [12] bands
       [13] channels
       [14] powerLevels
    */

    if (!payload || data_size < 15) {
        return; // malformed
    }
    (void)owner;

    const uint8_t vtx_type          = payload[0];
    uint8_t band_raw                = payload[1];
    uint8_t ch_raw                  = payload[2];
    uint8_t power_1based            = payload[3];
    uint8_t pitmode                 = payload[4];
    const uint16_t freq_mhz         = (uint16_t)payload[5] | ((uint16_t)payload[6] << 8);
    const uint8_t device_ready      = payload[7];
    const uint8_t low_power_disarm  = payload[8];
    uint16_t pit_mode_freq          = (uint16_t)payload[9] | ((uint16_t)payload[10] << 8);
    uint8_t vtx_table_available     = payload[11];
    uint8_t vtx_table_bands         = payload[12];
    uint8_t vtx_table_channels      = payload[13];
    uint8_t vtx_table_power_levels  = payload[14];

    if (!vtx_table_available) {
        return; // ignore if no VTX table
    }

    if (low_power_disarm) {
        power_1based = 1;
    }

    if (power_1based < 1) power_1based = 1;
    if ((unsigned)power_1based > g_vtx_power_level_count) power_1based = (uint8_t)g_vtx_power_level_count;

    g_cfg.pitmode = pitmode ? 1 : 0;
    g_cfg.power = (uint8_t)power_1based;

    g_cfg.channel = ch_raw;
    g_cfg.band = band_raw;

    if (band_raw) {
        g_cfg.frequency = g_bands[band_raw - 1].freq[ch_raw - 1];
    } else {
        g_cfg.frequency = freq_mhz;
    }

    if (vtx_table_bands != vtx_get_band_count() || vtx_table_power_levels != g_vtx_power_level_count) {
        g_cfg.vtx_table_available = 0;
    } else {
        g_cfg.vtx_table_available = vtx_table_available;
    }

    g_cfg.configSet = 1;

    static uint16_t last_freq;
    static uint8_t last_power;
    static uint8_t last_pitmode;
    if (last_freq != g_cfg.frequency || last_power != g_cfg.power || last_pitmode != g_cfg.pitmode) {
        vtx_apply_hw(&g_cfg);
        last_freq = g_cfg.frequency;
        last_power = g_cfg.power;
        last_pitmode = g_cfg.pitmode;
    }

    (void) vtx_type;
    (void) device_ready;
    (void) pit_mode_freq;
    (void) vtx_table_bands;
    (void) vtx_table_channels;
    (void) vtx_table_power_levels;
}

void vtx_msp_clear_table_and_set_defaults(uint8_t owner)
{
    uint8_t p[15] = {0};
    p[0]  = 0;
    p[1]  = 0;
    p[2]  = 1;
    p[3]  = 0;
    p[4]  = 0;
    p[5]  = 0; p[6]  = 0;
    p[7]  = vtx_get_band_count();
    p[8]  = VTX_CHANNEL_COUNT;
    p[9]  = 0; p[10] = 0;
    p[11] = vtx_get_band_count();
    p[12] = VTX_CHANNEL_COUNT;
    p[13] = g_vtx_power_level_count;
    p[14] = 1;

    uint8_t tx_buff[64];
    const uint16_t len = construct_msp_command_v1(tx_buff, MSP_SET_VTX_CONFIG, p, sizeof(p), MSP_OUTBOUND);
    msp_tx_send_owner(owner, tx_buff, len);

    vtx_msp_push_power_table(owner);
    vtx_msp_push_band_table(owner);
    vtx_msp_eeprom_write(owner);

    g_cfg.vtx_table_available = 1;
}

/* Power table
 * Send MSP_SET_VTXTABLE_POWERLEVEL for each entry.
 * Payload:
 *   [0] index (1..N)
 *   [1..2] power_mW (uint16 LE)
 *   [3] label_len
 *   [4..] ASCII label (e.g. "25","100","800")
 *
 * Label is generated here (snprintf from mW) */
void vtx_msp_push_power_table(uint8_t owner)
{
    for (uint8_t i = 1; i <= g_vtx_power_level_count; i++) {
        const uint16_t mw = g_vtx_power_levels[i - 1].mW;

        char label[16];
        int label_len = snprintf(label, sizeof(label), "%u", (unsigned)mw);
        if (label_len < 0) label_len = 0;
        if (label_len > (int)sizeof(label)) label_len = sizeof(label);

        uint8_t p[1 + 2 + 1 + 16] = {0};
        p[0] = i;
        p[1] = (uint8_t)(mw & 0xFF);
        p[2] = (uint8_t)((mw >> 8) & 0xFF);
        p[3] = (uint8_t)label_len;
        memcpy(&p[4], label, (size_t)label_len);

        uint8_t tx_buff[64];
        const uint16_t len = construct_msp_command_v1(tx_buff,
                            MSP_SET_VTXTABLE_POWERLEVEL,
                            p, (uint8_t)(4 + label_len),
                            MSP_OUTBOUND);

        msp_tx_send_owner(owner, tx_buff, len);
    }
}

void vtx_msp_push_band_table(uint8_t owner)
{
    for (uint8_t b = 1; b <= vtx_get_band_count(); b++) {
        const vtx_band_t *band = &g_bands[b-1];

        /* Payload layout (29 bytes):
           [0]=band(1..N), [1]=nameLen(=8), [2..9]=name8,
           [10]=letter, [11]=isFactory(1), [12]=channels(8),
           [13..28]=8×freq LE16
        */
        uint8_t p[29] = {0};
        p[0] = b;
        p[1] = VTX_CH_LABEL_COUNT;

        /* Name (exactly 8 bytes) */
        for (uint8_t i = 0; i < VTX_CH_LABEL_COUNT; i++) {
            p[2 + i] = band->band_name[i];
        }

        p[10] = (uint8_t)band->letter;      /* single ASCII letter */
        p[11] = 1;                          /* factory band flag */
        p[12] = VTX_CHANNEL_COUNT;

        for (uint8_t ch = 0; ch < VTX_CHANNEL_COUNT; ch++) {
            const uint16_t f = band->freq[ch];
            p[13 + ch*2 + 0] = (uint8_t)(f & 0xFF);
            p[13 + ch*2 + 1] = (uint8_t)(f >> 8);
        }

        uint8_t tx_buff[64];
        const uint16_t len = construct_msp_command_v2(tx_buff, MSP_SET_VTXTABLE_BAND, p, (uint8_t)sizeof(p), MSP_PACKET_COMMAND);

        msp_tx_send_owner(owner, tx_buff, len);
    }
}

#if defined(USE_PA)
/* Payload is variable-length, sized to the current layout (max 6 +
 * VTX_CAL_FREQ_POINTS_MAX*4 = 42 bytes, well inside MSPv2's limit):
 *   [0]  idx (1..g_vtx_power_level_count)
 *   [1-2] mW (u16 LE)
 *   [3]  ext_pa_enable (0/1)
 *   [4]  rtc6705_level (raw PA5G_PW register value)
 *   [5]  freq_point_count -- echoed so a client can decode this payload
 *        without a separate layout query first
 *   [6..] calibration[freq_point_count] (u16 LE each)
 *   [..]  detector[freq_point_count] (u16 LE each)
 * mW/ext_pa_enable/rtc6705_level are genuine current values now (all
 * three are settable via vtx_msp_set_calibration_table()), not fixed
 * hardware facts as before. */
void vtx_msp_push_calibration_table(uint8_t owner)
{
    for (uint8_t i = 1; i <= g_vtx_power_level_count; i++) {
        const vtx_power_level_t *lvl = &g_vtx_power_levels[i - 1];

        uint8_t p[6 + VTX_CAL_FREQ_POINTS_MAX * 4] = {0};
        p[0] = i;
        p[1] = (uint8_t)(lvl->mW & 0xFF);
        p[2] = (uint8_t)((lvl->mW >> 8) & 0xFF);
        p[3] = lvl->ext_pa_enable ? 1 : 0;
        p[4] = (uint8_t)lvl->rtc6705_level;
        p[5] = g_vtx_cal_freq_point_count;

        uint8_t off = 6;
        for (uint8_t c = 0; c < g_vtx_cal_freq_point_count; c++) {
            p[off++] = (uint8_t)(lvl->calibration[c] & 0xFF);
            p[off++] = (uint8_t)((lvl->calibration[c] >> 8) & 0xFF);
        }
        for (uint8_t c = 0; c < g_vtx_cal_freq_point_count; c++) {
            p[off++] = (uint8_t)(lvl->detector[c] & 0xFF);
            p[off++] = (uint8_t)((lvl->detector[c] >> 8) & 0xFF);
        }

        uint8_t tx_buff[64];
        const uint16_t len = construct_msp_command_v2(tx_buff,
                            MSP_SET_PACALTABLE,
                            p, off,
                            MSP_PACKET_COMMAND);

        msp_tx_send_owner(owner, tx_buff, len);
    }
}

/* Two payload shapes:
 *   data_size == 1, payload[0] == 0xFF: factory reset -- level count,
 *     frequency breakpoints AND every level's mW/hardware mapping/
 *     calibration/detector all go back to this target's own compiled-in
 *     defaults (g_vtx_power_level_defaults[]/g_vtx_cal_freq_defaults_mhz[],
 *     from this board's target.c) -- NOT a hardcoded/zeroed value, since
 *     what counts as "safe/uncalibrated" depends on PA_DAC_SIGN and
 *     varies by target (a naive 0 would be actively dangerous on an
 *     inverted-sign board, where low DAC = high output). See
 *     vtx_power_levels_reset_to_defaults().
 *   data_size >= 6: normal per-level write -- see this function's own
 *     payload doc comment above (on vtx_msp_push_calibration_table()).
 *     Rejected (logged, no state change) if freq_point_count doesn't
 *     match the table's current layout, rather than misinterpreting the
 *     calibration[]/detector[] arrays under the wrong point count.
 * Anything else is malformed and ignored. */
void vtx_msp_set_calibration_table(uint8_t owner, const uint8_t *payload, uint16_t data_size)
{
    (void)owner;
    if (!payload) {
        return; // malformed
    }

    if (data_size == 1) {
        if (payload[0] == 0xFF) {
            vtx_power_levels_reset_to_defaults();
        }
        return;
    }

    if (data_size < 6) {
        return; // malformed
    }

    const uint16_t level = payload[0];
    const uint8_t freq_point_count = payload[5];

    if (!level || level > g_vtx_power_level_count) {
        return; // out of range for the current layout
    }
    if (freq_point_count != g_vtx_cal_freq_point_count) {
        TRACE_ERROR("Rejected PA table entry, frequency count mismatch. got: %u, expected: %u\n", freq_point_count, g_vtx_cal_freq_point_count);
        return; // stale/mismatched client -- refuse rather than misread the arrays below
    }
    if (data_size < (uint16_t)(6 + freq_point_count * 2)) {
        return; // malformed
    }

    vtx_power_level_t *lvl = &g_vtx_power_levels[level - 1];
    lvl->mW = (uint16_t)payload[1] | ((uint16_t)payload[2] << 8);
    lvl->ext_pa_enable = payload[3] != 0;
    lvl->rtc6705_level = (rtc6705_power_t)payload[4];

    for (uint8_t c = 0; c < freq_point_count; c++) {
        lvl->calibration[c] = (uint16_t)payload[6 + c * 2] | ((uint16_t)payload[7 + c * 2] << 8);
    }

    if (data_size >= (uint16_t)(6 + freq_point_count * 4)) {
        const uint16_t det_off = 6 + freq_point_count * 2;
        for (uint8_t c = 0; c < freq_point_count; c++) {
            lvl->detector[c] = (uint16_t)payload[det_off + c * 2] | ((uint16_t)payload[det_off + c * 2 + 1] << 8);
        }
    }

    TRACE_INFO("SET PA table %i\n", level);
    vtx_power_levels_write_eeprom((uint8_t)level);
}

/* Query reply (sent for a zero-length MSP_VTX_POWER_TABLE_LAYOUT request,
 * as well as unprompted as the immediate ACK to a SET -- before anything
 * is actually written to EEPROM, see vtx_power_levels_flush_if_dirty()):
 *   [0] schema_version
 *   [1] power_level_count
 *   [2] power_level_max (VTX_POWER_LEVEL_MAX)
 *   [3] freq_point_count
 *   [4] freq_point_max (VTX_CAL_FREQ_POINTS_MAX)
 *   [5] dac_sign_inverted (0/1) -- 1 if PA_DAC_SIGN > 0 (inverted -- lower
 *       DAC mV means MORE RF output, e.g. RTC76401), 0 if normal/typical.
 *       A calibration tool needs this to know which direction to step
 *       the DAC during a sweep; this table-wide hardware fact used to be
 *       smuggled into the PACALTABLE idx==0 row, which no longer exists.
 *   [6] layout_dirty (0/1) -- true while a level/layout change is staged
 *       in RAM but not yet flushed to EEPROM (vtx_power_levels_is_dirty()).
 *       A calibration tool polls this instead of guessing how long that
 *       write might take.
 *   [7..] frequencies_mhz[freq_point_count] (u16 LE each)
 */
void vtx_msp_push_power_table_layout(uint8_t owner)
{
    uint8_t p[7 + VTX_CAL_FREQ_POINTS_MAX * 2] = {0};
    p[0] = VTX_POWER_TABLE_SCHEMA_VERSION;
    p[1] = g_vtx_power_level_count;
    p[2] = VTX_POWER_LEVEL_MAX;
    p[3] = g_vtx_cal_freq_point_count;
    p[4] = VTX_CAL_FREQ_POINTS_MAX;
    p[5] = (PA_DAC_SIGN > 0) ? 1 : 0;
    p[6] = vtx_power_levels_is_dirty() ? 1 : 0;

    uint8_t off = 7;
    for (uint8_t c = 0; c < g_vtx_cal_freq_point_count; c++) {
        p[off++] = (uint8_t)(g_vtx_cal_frequencies_mhz[c] & 0xFF);
        p[off++] = (uint8_t)((g_vtx_cal_frequencies_mhz[c] >> 8) & 0xFF);
    }

    // construct_msp_command_v2() needs 3 (header) + 5 (v2 header) + off
    // (payload) + 1 (checksum) bytes; off maxes out at 7 + 9*2 = 25, so
    // this must be at least 34 -- sized to 64 to match this file's other
    // MSP send buffers with headroom.
    uint8_t tx_buff[64];
    const uint16_t len = construct_msp_command_v2(tx_buff,
                        MSP_VTX_POWER_TABLE_LAYOUT,
                        p, off,
                        MSP_PACKET_COMMAND);

    msp_tx_send_owner(owner, tx_buff, len);
}

/* Set request payload:
 *   [0] power_level_count (1..VTX_POWER_LEVEL_MAX)
 *   [1] freq_point_count (2..VTX_CAL_FREQ_POINTS_MAX)
 *   [2..] frequencies_mhz[freq_point_count] (u16 LE each), strictly
 *        ascending, each within 5600-6000 MHz
 * Validated by vtx_power_levels_set_layout() -- an invalid request is
 * rejected (logged, no state change) rather than silently clamped, and a
 * valid one ALWAYS resets every level's calibration[]/detector[] back to
 * target defaults (see that function's own doc comment for why a
 * changed frequency table can't keep old calibration data). Always
 * echoes the resulting layout back, so the caller can confirm what was
 * actually applied (or that a rejected request left the old layout in
 * place). */
void vtx_msp_set_power_table_layout(uint8_t owner, const uint8_t *payload, uint16_t data_size)
{
    if (!payload || data_size < 2) {
        vtx_msp_push_power_table_layout(owner); // malformed -- report current layout unchanged
        return;
    }

    const uint8_t power_level_count = payload[0];
    const uint8_t freq_point_count = payload[1];

    if (data_size < (uint16_t)(2 + freq_point_count * 2)) {
        vtx_msp_push_power_table_layout(owner); // malformed -- report current layout unchanged
        return;
    }

    uint16_t frequencies_mhz[VTX_CAL_FREQ_POINTS_MAX] = {0};
    for (uint8_t c = 0; c < freq_point_count && c < VTX_CAL_FREQ_POINTS_MAX; c++) {
        frequencies_mhz[c] = (uint16_t)payload[2 + c * 2] | ((uint16_t)payload[3 + c * 2] << 8);
    }

    vtx_power_levels_set_layout(power_level_count, freq_point_count, frequencies_mhz);
    vtx_msp_push_power_table_layout(owner);
}

/* Payload is 11 bytes -- atomicity of this data is required by the calibration tool
 * instead of needing to stitch this information together from several separate
 * queries that could each be answered at a slightly different moment:
 *   [0] power
 *   [1-2] pa Vref for RF Vbias - the DAC output voltage (mV, LSB/MSB)
 *   [3-4] pa Vdetector (mV, LSB/MSB)
 *   [5] boost_on (0/1) -- rf_pa_boost_is_on()
 *   [6] rtc6705_level (raw PA5G_PW register value, 0-3) -- rtc6705_get_power()
 *   [7] pid_active (0/1) -- rf_pa_pid_active(), the loop's own real gate
 *       condition, not an approximation of it
 *   [8-9] frequency (MHz, LSB/MSB) -- g_cfg.frequency, duplicated here
 *       from what MSP_VTX_CONFIG already reports, specifically so this
 *       stays a single atomic read
 *   [10] session_active (0/1) -- rf_pa_calibration_session_is_active(),
 *       see rf_pa_calibration_session_begin()'s doc comment in rf_pa.h
 *   [11-12] mcu_temp_c_x10 (signed, tenths of a degree C, LSB/MSB) --
 *       adc_read_mcu_temp_c() * 10.
 *   [13-14] ntc_raw (12-bit ADC code, LSB/MSB) -- rf_pa_read_ntc_raw().
 *       0 on a board with no NTC (see that function's own fallback in
 *       rf_pa.c) -- not a real reading in that case, not distinguishable
 *       from a genuine 0 code, but that's the same "0 means nothing to
 *       report" convention the rest of this reply already uses elsewhere.
 *   [15-16] pa_temp_c_x10 (signed, tenths of a degree C, LSB/MSB) --
 *       rf_pa_read_ntc_temp_c() * 10. Also 0 on a board with no NTC.
 */
void vtx_msp_push_calibration(uint8_t owner)
{
    uint8_t p[17] = {0};
    uint16_t detector_mv = (uint16_t)rf_pa_get_detector_mv();
    int16_t mcu_temp_c_x10 = (int16_t)(adc_read_mcu_temp_c() * 10.0f);
    uint16_t ntc_raw = rf_pa_read_ntc_raw();
    int16_t pa_temp_c_x10 = (int16_t)(rf_pa_read_ntc_temp_c() * 10.0f);

    p[0] = g_cfg.power;
    p[1] = (uint8_t)(rf_pa_get_vref_mv() & 0xFF);
    p[2] = (uint8_t)((rf_pa_get_vref_mv() >> 8) & 0xFF);
    p[3] = (uint8_t)(detector_mv & 0xFF);
    p[4] = (uint8_t)((detector_mv >> 8) & 0xFF);
    p[5] = rf_pa_boost_is_on() ? 1 : 0;
    p[6] = (uint8_t)rtc6705_get_power();
    p[7] = rf_pa_pid_active() ? 1 : 0;
    p[8] = (uint8_t)(g_cfg.frequency & 0xFF);
    p[9] = (uint8_t)((g_cfg.frequency >> 8) & 0xFF);
    p[10] = rf_pa_calibration_session_is_active() ? 1 : 0;
    p[11] = (uint8_t)((uint16_t)mcu_temp_c_x10 & 0xFF);
    p[12] = (uint8_t)(((uint16_t)mcu_temp_c_x10 >> 8) & 0xFF);
    p[13] = (uint8_t)(ntc_raw & 0xFF);
    p[14] = (uint8_t)((ntc_raw >> 8) & 0xFF);
    p[15] = (uint8_t)((uint16_t)pa_temp_c_x10 & 0xFF);
    p[16] = (uint8_t)(((uint16_t)pa_temp_c_x10 >> 8) & 0xFF);

    uint8_t tx_buff[32];
    const uint16_t len = construct_msp_command_v2(tx_buff,
                        MSP_PACALIBRATION,
                        p, (uint8_t)sizeof(p),
                        MSP_PACKET_COMMAND);

    msp_tx_send_owner(owner, tx_buff, len);
}

/* Payload is at least 3 bytes.
 *   [0] level
 *   [1-2] mv_lo, mv_hi
 *   [3] session_active (0/1) -- mirrors the tool's own current session
 *       state; see rf_pa_calibration_session_begin()'s doc comment in
 *       rf_pa.h for what a session actually changes. Edge-triggered
 *       against rf_pa_calibration_session_is_active() so the common
 *       case (this byte matching what's already active) is a no-op,
 *       not a begin()/end() call on every single calibration point.
 *   [4] boost_mode (0=off, 1=on, 2=auto/ext_pa_enable-driven) -- see
 *       rf_pa_manual_boost_set()'s doc comment in rf_pa.h. Also edge-
 *       triggered, against rf_pa_manual_boost_override_active() and
 *       rf_pa_boost_is_on().
 * Both fields are omittable (data_size 3 or 4) for backward
 * compatibility -- a shorter payload simply leaves that piece of state
 * untouched rather than erroring. */
void vtx_msp_set_calibration(uint8_t owner, const uint8_t *payload, uint16_t data_size)
{
    static uint8_t counter = 49;

    if (!payload || data_size < 3) {
        return; // malformed
    }
    (void)owner;

    uint8_t level = payload[0];
    uint16_t pa_mv = payload[1] + (uint16_t)(payload[2] << 8);

    if (level && level != g_cfg.power && level <= g_vtx_power_level_count) {
        g_cfg.power = level;
        rtc6705_allow_power_writes(true);
        rtc6705_set_power(g_vtx_power_levels[g_cfg.power - 1].rtc6705_level);
        rtc6705_allow_power_writes(false);
        TRACE_INFO("Calibration power %i\n", level);
    }

    if (pa_mv) {
        rf_pa_set_calibration(pa_mv); // suspends the closed loop for this level until the next rf_pa_apply_level() -- see rf_pa.h
        if (!counter--) {
            TRACE_INFO("Calibration mv %i\n", pa_mv);
            counter = 49;
        }
    }

    if (data_size >= 4) {
        bool want_session = payload[3] != 0;
        if (want_session != rf_pa_calibration_session_is_active()) {
            if (want_session) {
                rf_pa_calibration_session_begin();
                TRACE_INFO("Calibration session begin\n");
            } else {
                rf_pa_calibration_session_end();
                TRACE_INFO("Calibration session end\n");
            }
        }
    }

    if (data_size >= 5) {
        uint8_t boost_mode = payload[4];
        if (boost_mode == 2) {
            if (rf_pa_manual_boost_override_active()) {
                rf_pa_manual_boost_clear();
                TRACE_INFO("Manual PA boost override cleared\n");
            }
        } else {
            bool want_on = boost_mode != 0;
            if (!rf_pa_manual_boost_override_active() || want_on != rf_pa_boost_is_on()) {
                rf_pa_manual_boost_set(want_on);
                TRACE_INFO("Manual PA boost %s\n", want_on ? "on" : "off");
            }
        }
    }

    vtx_msp_push_calibration(owner);
}
#endif //USE_PA

/* Responds to an empty-payload MSP_VTX_CONFIG (a QUERY, not a push) with
 * current status, matching Betaflight's real query/response semantics --
 * handle_msp_set_vtx_config() only handles the "FC is pushing new
 * config" direction (payload >= 15 bytes); a zero-length request was
 * previously just silently ignored, so nothing could ever read current
 * state back. p[0] (vtxType) = 5 = VTXDEV_MSP, Betaflight's own enum
 * value for an MSP-controlled VTX (matches "VTX Type: MSP" in
 * Betaflight's own VTX config UI). */
static void vtx_msp_push_vtx_config(uint8_t owner)
{
    uint8_t p[15] = {0};
    p[0]  = 5; // VTXDEV_MSP
    p[1]  = g_cfg.band;
    p[2]  = g_cfg.channel;
    p[3]  = g_cfg.power;
    p[4]  = g_cfg.pitmode;
    p[5]  = (uint8_t)(g_cfg.frequency & 0xFF);
    p[6]  = (uint8_t)((g_cfg.frequency >> 8) & 0xFF);
    p[7]  = g_cfg.configSet ? 1 : 0; // deviceIsReady
    p[8]  = 0;                       // lowPowerDisarm -- not tracked currently
    p[9]  = 0; p[10] = 0;            // pitModeFreq -- not tracked currently
    p[11] = g_cfg.vtx_table_available;
    p[12] = vtx_get_band_count();
    p[13] = VTX_CHANNEL_COUNT;
    p[14] = g_vtx_power_level_count;

    uint8_t tx_buff[64];
    const uint16_t len = construct_msp_command_v1(tx_buff, MSP_VTX_CONFIG, p, sizeof(p), MSP_INBOUND); // MSP_INBOUND -> '>' response marker
    msp_tx_send_owner(owner, tx_buff, len);
}

void vtx_msp_eeprom_write(uint8_t owner)
{
    uint8_t tx_buff[64];
    const uint16_t len = construct_msp_command_v1(tx_buff, MSP_EEPROM_WRITE, NULL, 0, MSP_OUTBOUND);
    msp_tx_send_owner(owner, tx_buff, len);
}

void vtx_msp_request_config(uint8_t owner)
{
    uint8_t tx_buff[64];
    const uint16_t len = construct_msp_command_v1(tx_buff, MSP_VTX_CONFIG, NULL, 0, MSP_OUTBOUND);
    msp_tx_send_owner(owner, tx_buff, len);
}

bool vtx_msp_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{
    switch (msp_cmd) {
    case MSP_VTX_CONFIG:
        if (data_size == 0) {
            vtx_msp_push_vtx_config(owner);
        } else {
            handle_msp_set_vtx_config(owner, payload, data_size);
        }
        break;

#if defined(USE_PA)
    case MSP_PACALTABLE:
        vtx_msp_push_calibration_table(owner);
        break;

    case MSP_PACALIBRATION:
        if (data_size == 0) {
            vtx_msp_push_calibration(owner);
        }
        break;

    case MSP_SET_PACALIBRATION:
        vtx_msp_set_calibration(owner, payload, data_size);
        break;

    case MSP_SET_PACALTABLE:
        vtx_msp_set_calibration_table(owner, payload, data_size);
        break;

    case MSP_VTX_POWER_TABLE_LAYOUT:
        if (data_size == 0) {
            vtx_msp_push_power_table_layout(owner);
        }
        break;

    case MSP_SET_VTX_POWER_TABLE_LAYOUT:
        vtx_msp_set_power_table_layout(owner, payload, data_size);
        break;
#endif

    case MSP_EEPROM_WRITE:
        eeprom_save();
        break;

    case MSP_SET_VTX_CONFIG:
    case MSP_VTXTABLE_BAND:
    case MSP_VTXTABLE_POWERLEVEL:
    default:
        return false;
    }

    const vtx_config_t *vtx_config = vtx_get_config();
    if (!vtx_config->vtx_table_available) {
        TRACE_INFO("Set Table defaults\n");
        vtx_msp_clear_table_and_set_defaults(owner);
    }
    return true;
}
#endif
