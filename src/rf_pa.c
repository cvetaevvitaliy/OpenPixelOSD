/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"
#include "rf_pa.h"
#include "vtx_msp.h"
#include "canvas_char.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#if defined(USE_PA)

/* Gains operate on a VDET deviation in mV (rf_detector_target - rf_detector,
 * both mV). Board-specific -- each USE_PA target header (e.g.
 * targets/generic_vtx_pa_rtc76401.h) MUST define all three; there's no
 * value that's a genuine no-op for a PID gain (Kp=1 still very much
 * drives the loop, and Kp=Ki=Kd=0 is really just "silently disabled"
 * wearing a different hat) -- so a missing definition is a compile
 * error rather than a silently-inherited default. */
#if !defined(PA_CONTROL_Kp)
#error "PA_CONTROL_Kp not defined -- set it in your board's target header (see targets/generic_vtx_pa_rtc76401.h)"
#endif
#if !defined(PA_CONTROL_Ki)
#error "PA_CONTROL_Ki not defined -- set it in your board's target header (see targets/generic_vtx_pa_rtc76401.h)"
#endif
#if !defined(PA_CONTROL_Kd)
#error "PA_CONTROL_Kd not defined -- set it in your board's target header (see targets/generic_vtx_pa_rtc76401.h)"
#endif

/* Hard bounds on what the closed loop is ever allowed to command,
 * regardless of gains or how wrong/unreachable a detector[] target turns
 * out to be. Board-specific -- MUST match the DAC range you've actually
 * bench-validated as safe, not just "0..3300 because that's the DAC's
 * physical range". A missing definition is a compile error for the same
 * reason as the gains above: there's no universally-safe default. */
#if !defined(PA_CONTROL_MV_MIN)
#error "PA_CONTROL_MV_MIN not defined -- set it in your board's target header, to the lowest DAC mV you've bench-confirmed safe with the boost stage on"
#endif
#if !defined(PA_CONTROL_MV_MAX)
#error "PA_CONTROL_MV_MAX not defined -- set it in your board's target header"
#endif
#if !defined(PA_CONTROL_I_CLAMP_MV)
#error "PA_CONTROL_I_CLAMP_MV not defined -- set it in your board's target header (bounds the integral term's own contribution, independent of the final output clamp)"
#endif

/* Which direction increases RF output. NOT universal across PA designs:
 * RTC76401 (P-channel gate injection into RTC6705's PAOUT1) is INVERTED -- lower
 * DAC mV = more conduction = more output.
 * A more typical linear PA bias scheme would be the opposite (higher DAC = more bias = more output).
 * +1.0f = inverted (RTC76401's proven behavior); -1.0f = normal/typical.
 * No safe default -- a missing or wrong value here makes the closed loop
 * correct in the wrong direction */
#if !defined(PA_DAC_SIGN)
#error "PA_DAC_SIGN not defined -- set it in your board's target header: +1.0f if lower DAC mV means MORE RF output (e.g. RTC76401's Q2 gate injection), -1.0f if higher DAC mV means more output (typical linear bias)"
#endif

/* Additive offset, not a gain -- 0 is a genuine no-op (adds nothing), so
 * a fallback default is fine here unlike the three gains above. */
#ifndef PA_CONTROL_OFFSET_MV
#define PA_CONTROL_OFFSET_MV 0u
#endif

/* The safe VBIAS floor while the PA IS actively enabled and under
 * closed-loop control -- the end of [PA_CONTROL_MV_MIN,
 * PA_CONTROL_MV_MAX] that corresponds to LEAST RF output for this
 * board's sign.
 */
#define PA_VBIAS_MIN_MV ((PA_DAC_SIGN > 0) ? PA_CONTROL_MV_MAX : PA_CONTROL_MV_MIN)

/* The DAC value while the PA/boost is disabled entirely, Sign aware. */
#define VTX_BIAS_OFF_MV ((PA_DAC_SIGN > 0) ? 3300u : 0u)

static uint16_t g_vref_mv = 0;
static uint16_t g_cal_mv_baseline = 0; // this level's calibration[] starting DAC value -- the PID trims AROUND this, it does not compute mv from scratch
static float    rf_detector_target = 0;
static double   rf_detector = 0;
static float    pa_control_i = 0;
static float    pa_control_last_deviation = 0;
static const vtx_power_level_t *g_active_level = NULL;
static bool g_calibration_override = false;             // true while rf_pa_set_calibration() has set an interactive override -- rf_pa_loop() must not fight it
static bool g_boost_on = false;                         // mirrors whichever of rf_pa_boost_on()/rf_pa_boost_off() ran last
static bool g_calibration_session_active = false;
static bool g_manual_boost_override_active = false;
static bool g_manual_boost_state = false;

/* A gate cap on the VBIAS FET needs time to charge before the FET -- and so the PA's
 * actual bias -- has stabilized. Enabling boost before this settles
 * produces a real power spike on the very first post-enable reading
 */
#define GATE_SETTLE_MS 20u

static inline void dac_ch2_write_mv(uint16_t mv)
{
    uint32_t dac_raw = DAC12BIT_FROM_MV(mv);
    if (dac_raw > 4095u) dac_raw = 4095u;
    LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_2, dac_raw);
    LL_DAC_TrigSWConversion(DAC1, LL_DAC_CHANNEL_2);
    g_vref_mv = mv;
}

/* External/boost PA GPIO control only -- does NOT touch the DAC bias.
 * No-op on boards without a separate boost-enable pin (e.g. PA_GENERIC).
 * Callers are expected to have already written the DAC to its intended
 * value before calling this.
 *
 * see GATE_SETTLE_MS too
 */
static inline void rf_pa_boost_on(void)
{
    if (!g_boost_on) {
        HAL_Delay(GATE_SETTLE_MS);
    }
#if defined(PA_ON_Pin)
    #if !defined(PA_ON_ACTIVE_LOW)
    LL_GPIO_SetOutputPin(PA_ON_GPIO_Port, PA_ON_Pin);
    #else
    LL_GPIO_ResetOutputPin(PA_ON_GPIO_Port, PA_ON_Pin);
    #endif
#endif
    g_boost_on = true;
}

static inline void rf_pa_boost_off(void)
{
#if defined(PA_ON_Pin)
    #if !defined(PA_ON_ACTIVE_LOW)
    LL_GPIO_ResetOutputPin(PA_ON_GPIO_Port, PA_ON_Pin);
    #else
    LL_GPIO_SetOutputPin(PA_ON_GPIO_Port, PA_ON_Pin);
    #endif
#endif
    g_boost_on = false;
}

static float lerp(float x, float in_min, float in_max, float out_min, float out_max)
{
    if (in_max == in_min) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static uint8_t cal_freq_index(uint16_t freq)
{
    if (freq < g_vtx_power_levels[0].calibration[0]) freq = g_vtx_power_levels[0].calibration[0];
    if (freq > g_vtx_power_levels[0].calibration[VTX_CAL_FREQ_POINTS - 1]) freq = g_vtx_power_levels[0].calibration[VTX_CAL_FREQ_POINTS - 1];
    for (uint8_t i = 0; i < VTX_CAL_FREQ_POINTS - 1; i++) {
        if (freq < g_vtx_power_levels[0].calibration[i + 1]) return i;
    }
    return VTX_CAL_FREQ_POINTS - 2;
}

static uint16_t get_calibration_mv(const vtx_power_level_t *lvl, uint16_t freq)
{
    uint8_t i = cal_freq_index(freq);
    return (uint16_t)lerp(freq, g_vtx_power_levels[0].calibration[i], g_vtx_power_levels[0].calibration[i + 1],
                           lvl->calibration[i], lvl->calibration[i + 1]);
}

static uint16_t get_detector_target(const vtx_power_level_t *lvl, uint16_t freq)
{
    uint8_t i = cal_freq_index(freq);
    return (uint16_t)lerp(freq, g_vtx_power_levels[0].calibration[i], g_vtx_power_levels[0].calibration[i + 1],
                           lvl->detector[i], lvl->detector[i + 1]);
}

void rf_pa_init(void)
{
    /* Enable DAC1 ch2 if not already enabled by user init (DAC1_Init()
     * in dac.c, called from video_overlay_init(), already configured
     * PA5 as analog/buffered/GPIO-connected -- this just arms channel 2
     * and zeroes it). */
    LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_2);

#if defined(PA_RTC76401)
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB); /* harmless if already on */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC); /* harmless if already on */
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = PA_ON_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(PA_ON_GPIO_Port, &GPIO_InitStruct);
#endif

    rf_pa_disable(); // keep PA off at boot
}

void rf_pa_disable(void)
{
    g_active_level = NULL;
    rf_pa_boost_off();
    dac_ch2_write_mv(VTX_BIAS_OFF_MV);
    g_cal_mv_baseline = VTX_BIAS_OFF_MV;
    rf_detector_target = 0;
    rf_detector = 0;
    pa_control_i = 0;
    pa_control_last_deviation = 0;
}

void rf_pa_apply_level(const vtx_power_level_t *lvl)
{
    g_active_level = lvl;

    if (!lvl) {
        // Always fully honored, even mid-session -- explicitly turning
        // the PA off is a safety operation and must never be suppressed.
        g_calibration_override = false;
        rf_pa_boost_off();
        dac_ch2_write_mv(VTX_BIAS_OFF_MV);
        g_cal_mv_baseline = VTX_BIAS_OFF_MV;
        rf_detector_target = 0;
        rf_detector = 0;
        pa_control_i = 0;
        pa_control_last_deviation = 0;
        return;
    }

    if (g_calibration_session_active) {
        // Don't reset the DAC to the level's default calibration[]
        // baseline, and don't re-arm the closed loop's detector[]
        // target -- THAT'S the pair that actually fights a calibration
        // tool's override (see rf_pa_calibration_session_begin()'s doc
        // comment in rf_pa.h).
        //
        // Boost still needs restoring here regardless of session state
        if (g_manual_boost_override_active) {
            if (g_manual_boost_state) {
                rf_pa_boost_on();
            } else {
                rf_pa_boost_off();
            }
        } else if (lvl->ext_pa_enable) {
            rf_pa_boost_on();
        } else {
            rf_pa_boost_off();
        }
        return;
    }

    g_calibration_override = false; // returning to normal, table-driven operation

    uint16_t freq = vtx_get_config()->frequency;

    rf_detector_target = get_detector_target(lvl, freq);
    // Without this, rf_detector stays at whatever vtx_apply_hw()'s own
    // preceding, unconditional rf_pa_apply_level(NULL) call just left it
    // at (0 -- that call's own !lvl branch zeroes it) for this level's
    // very first rf_pa_loop() tick. error = (rf_detector -
    // rf_detector_target) * PA_DAC_SIGN would then be computed against
    // a detector reading that was never real, not stale data from the
    // previous level -- a large, entirely artificial error the loop
    // reacts to immediately. Confirmed on a real run: switching from
    // level 7 (50mW) to level 6 (25mW) spiked to ~392mW for a few
    // hundred ms (an inverted-sign board driving the DAC hard toward
    // more power to "correct" the phantom error) before the EMA filter
    // caught up to reality and the loop settled back down. Seeding
    // rf_detector to the fresh target here means this level's first
    // tick starts from zero error instead, and only real readings (via
    // the EMA filter above) move it from there.
    rf_detector = rf_detector_target;
    pa_control_i = 0;
    pa_control_last_deviation = 0;

    g_cal_mv_baseline = get_calibration_mv(lvl, freq);
    dac_ch2_write_mv(g_cal_mv_baseline);

    if (lvl->ext_pa_enable) {
        rf_pa_boost_on();
    } else {
        rf_pa_boost_off();
    }
    /* if rf_detector_target != 0, rf_pa_loop() takes over trimming the
     * DAC bias AROUND g_cal_mv_baseline from here */
}

void rf_pa_restore(void)
{
    rf_pa_apply_level(g_active_level); // NULL-safe: re-applies "off" if that was the last state
    /* During an active calibration session (see rf_pa_calibration_
     * session_begin()), rf_pa_apply_level() above is a near no-op for a
     * real level -- it does NOT clear g_calibration_override or reset
     * to the level's table-driven baseline, so a retune leaves the PA
     * exactly where the calibration tool's last override put it (off,
     * per rf_pa_disable()'s own gating just before this call runs).
     * Outside a session, this behaves as it always has: clears the
     * override and drops back to the level's table-driven baseline
     */
}

void rf_pa_set_vref_mv(uint16_t mv)
{
    dac_ch2_write_mv(mv);
}

void rf_pa_set_calibration(uint16_t mv)
{
    g_calibration_override = true;
    dac_ch2_write_mv(mv);
}

double rf_pa_get_detector_mv(void)
{
    return rf_detector;
}

uint16_t rf_pa_get_vref_mv(void)
{
    return g_vref_mv;
}

bool rf_pa_boost_is_on(void)
{
    return g_boost_on;
}

/* True exactly when rf_pa_loop() would actually act this tick if called
 * right now -- mirrors its own gate (rf_detector_target &&
 * !g_calibration_override) precisely, rather than approximating it, so
 * a calibration tool can tell "the loop is active and could fight an
 * override I'm about to send" apart from "conditions look inactive". */
bool rf_pa_pid_active(void)
{
    return g_active_level != NULL && rf_detector_target != 0 && !g_calibration_override;
}

bool rf_pa_calibration_override_active(void)
{
    return g_calibration_override;
}

/* See rf_pa_calibration_session_begin()'s doc comment in rf_pa.h. */
void rf_pa_calibration_session_begin(void)
{
    g_calibration_session_active = true;
    g_manual_boost_override_active = false; // starts each session at automatic mode's own ext_pa_enable-driven default; Manual mode explicitly overrides right after, if it's the one starting
}

void rf_pa_calibration_session_end(void)
{
    g_calibration_session_active = false;
    g_manual_boost_override_active = false; // defensive: never leave a stale override in place once the session (and whatever was driving it) is gone
    rf_pa_restore(); // re-applies g_active_level via the NOW-normal (non-session) path -- see this function's own doc comment for why that's what actually resumes the closed loop
}

bool rf_pa_calibration_session_is_active(void)
{
    return g_calibration_session_active;
}

/* See rf_pa_manual_boost_set()'s doc comment in rf_pa.h. Takes effect
 * immediately (not just on the next retune), since a calibration tool
 * flipping this checkbox expects the PA to actually respond right away. */
void rf_pa_manual_boost_set(bool on)
{
    g_manual_boost_override_active = true;
    g_manual_boost_state = on;
    if (on) {
        rf_pa_boost_on();
    } else {
        rf_pa_boost_off();
    }
}

void rf_pa_manual_boost_clear(void)
{
    g_manual_boost_override_active = false;
}

bool rf_pa_manual_boost_override_active(void)
{
    return g_manual_boost_override_active;
}


uint16_t rf_pa_read_vdet_mv(void)
{
    return ADC_PA_VDET_READ_MV();
}

#if defined(ADC_NTC_INSTANCE)
/* Assumed circuit (see targets/generic_vtx_pa.h, the reference target
 * this was written against): VDDA --[10k pullup]-- ADC_node --[10k NTC]--
 * GND, with a small capacitor from ADC_node to GND for noise filtering.
 * The cap only affects settling time / noise rejection -- this is a
 * DC/steady-state ratio, so it doesn't appear in the math at all.
 *
 *   raw/4095 = V_adc/VDDA                          (ADC transfer function)
 *   V_adc/VDDA = R_ntc/(R_pullup+R_ntc)             (voltage divider)
 *   => raw/4095 = R_ntc/(R_pullup+R_ntc)
 *   => R_ntc = R_pullup * raw / (4095 - raw)
 *
 * Beta equation (NOT full Steinhart-Hart) converts that resistance to a
 * temperature -- accurate to a degree or two over roughly -20C..+85C for
 * a typical 10k NTC, which is plenty for PA thermal monitoring:
 *
 *   1/T = 1/T0 + (1/Beta) * ln(R_ntc/R0)
 *
 * NTC_BETA=3950 is a common value for a 10k NTC but varies by part and
 * manufacturer -- check your actual NTC's datasheet and update it, or
 * this will be systematically off (a wrong Beta shifts the whole curve,
 * it doesn't just add noise). */
#define NTC_R0_OHM     10000.0f   // NTC resistance at 25C (R25)
#define NTC_T0_K       298.15f    // 25C in Kelvin
#define NTC_BETA       3950.0f    // verify against your NTC's actual datasheet
#define NTC_PULLUP_OHM 10000.0f
#define NTC_ADC_FULL_SCALE 4095u  // 12-bit

float rf_pa_ntc_raw_to_celsius(uint16_t adc_raw)
{
    // At or above full-scale reads as an open circuit (NTC disconnected,
    // or a genuinely implausible near-zero resistance) -- the
    // divide-by-zero/negative-resistance case the formula below would
    // hit otherwise. -273.15C (absolute zero) is not physically
    // reachable, so it's used here as an unambiguous "invalid" sentinel
    // rather than NaN or some in-range-looking garbage value.
    if (adc_raw >= NTC_ADC_FULL_SCALE) {
        return -273.15f;
    }

    const float r_ntc = NTC_PULLUP_OHM * (float)adc_raw / (float)(NTC_ADC_FULL_SCALE - adc_raw);

    const float inv_t_kelvin = (1.0f / NTC_T0_K) + (1.0f / NTC_BETA) * logf(r_ntc / NTC_R0_OHM);
    const float t_kelvin = 1.0f / inv_t_kelvin;

    return t_kelvin - 273.15f;
}

float rf_pa_read_ntc_temp_c(void)
{
    return rf_pa_ntc_raw_to_celsius(ADC_NTC_READ_RAW());
}
#endif // ADC_NTC_INSTANCE

void debug_pa_loop(float p, float i, float d, float error, uint16_t vdet_mv, uint16_t vbias_mv)
{
    canvas_char_clean();

    const uint8_t ROW_OFFSET = 10;
    char buffer[COLUMN_SIZE+1];
    for (uint8_t row = 0; row <=4; row++) {
        for (uint8_t column = 0; column <= 1; column++) {
            switch (column) {
                case 0: {
                    switch (row) {
                        case 0: snprintf(buffer, 30, "VPD T % 0.2f ", rf_detector_target); break;
                        case 1: snprintf(buffer, 30, "VPD D % 0.2f", (float)rf_detector); break;
                        case 2: snprintf(buffer, 30, "VPD I  %u", vdet_mv); break;
                        case 3: snprintf(buffer, 30, "VBIAS  %u", vbias_mv); break;
#if defined(ADC2_NEEDED)
                        case 4: {
                            bool adc_en, adc_rdy, dma_en;
                            uint16_t dma_remain;
                            adc2_vdet_debug_status(&adc_en, &adc_rdy, &dma_en, &dma_remain);
                            snprintf(buffer, 30, "ADC%d RDY%d DMA%d N%u", adc_en, adc_rdy, dma_en, dma_remain);
                        } break;
#endif
                        default:
                            continue;
                    }
                    canvas_char_write(1, ROW_OFFSET + row, buffer, strlen(buffer));
                } break;
                case 1: {
                    switch (row) {
                        case 0: snprintf(buffer, 30, "P % 0.2f", p); break;
                        case 1: snprintf(buffer, 30, "I % 0.2f", i); break;
                        case 2: snprintf(buffer, 30, "D % 0.2f", d); break;
                        case 3: snprintf(buffer, 30, "E % 0.2f", error); break;
                        default:
                            continue;
                    }
                    canvas_char_write(COLUMN_SIZE - strlen(buffer) - 1, ROW_OFFSET + row, buffer, strlen(buffer));


                } break;
            }
        }
    }

    canvas_char_draw_complete();
}

/**
 * @brief Call periodically (e.g. every main-loop iteration). Runs the DAC
 * bias PID loop against the active level's detector target (mV) -- only
 * when that level has a non-zero target, i.e. it's been calibrated with
 * a real VDET reading.
 *
 * Control law notes:
 *  - error = (rf_detector - rf_detector_target) * PA_DAC_SIGN. Board's
 *    PA_DAC_SIGN encodes which direction increases output (see its
 *    definition above) -- this is NOT universal across PA designs, unlike
 *    everything else in this loop. For RTC76401 (PA_DAC_SIGN=+1.0f,
 *    bench-confirmed), Q2 is P-channel so LOWER DAC mV means MORE
 *    conduction means MORE output; a positive raw deviation (producing
 *    too much) needs a HIGHER mv (less conduction), and +1.0f makes a
 *    straightforward "add the correction" PID come out correct for that
 *    inverted relationship. A board where higher DAC genuinely means more
 *    output would use -1.0f instead. Getting this wrong makes the loop
 *    correct in the wrong direction.
 *  - mv is computed as g_cal_mv_baseline + correction, NOT from the
 *    correction terms alone. The baseline anchors the loop to a
 *    known-reasonable starting point (this level's calibration[] value)
 *    so a bad/unreachable target degrades to "stuck near the baseline",
 *    not "computed from scratch, no relation to anything validated".
 *  - Both the integral term alone and the final output are clamped to
 *    PA_CONTROL_I_CLAMP_MV / [PA_CONTROL_MV_MIN, PA_CONTROL_MV_MAX].
 *    Without these, an unreachable target (error never crosses zero)
 *    causes the integral to grow without bound indefinitely -- this is
 *    exactly what happened when detector[] targets were set above this
 *    board's real achievable VDET ceiling: the DAC got dragged through
 *    its full range while chasing a target it could never reach.
 */
void rf_pa_loop(bool field_edge_flag)
{
    static uint32_t last_detector_loop = 0;
    static uint32_t last_control_loop = 0;

    static bool debug_update_requested = false;
    if (field_edge_flag) {
        debug_update_requested = true;
    }

    if ((HAL_GetTick() - last_detector_loop) >= 1) {
        rf_detector = rf_detector * 0.99 + rf_pa_read_vdet_mv() * 0.01;
        last_detector_loop = HAL_GetTick();
    }

    if (!g_active_level) {
        pa_control_i = 0;
        return;
    }

    if (rf_detector_target && !g_calibration_override && (HAL_GetTick() - last_control_loop) >= 5) {
        float error = (rf_detector - rf_detector_target) * PA_DAC_SIGN; // see PA_DAC_SIGN notes above
        float p = error * PA_CONTROL_Kp;

        pa_control_i += error * PA_CONTROL_Ki;
        if (pa_control_i > PA_CONTROL_I_CLAMP_MV)  pa_control_i = PA_CONTROL_I_CLAMP_MV;
        if (pa_control_i < -PA_CONTROL_I_CLAMP_MV) pa_control_i = -PA_CONTROL_I_CLAMP_MV;

        float d = (error - pa_control_last_deviation) * PA_CONTROL_Kd;

        float mv = (float)g_cal_mv_baseline + PA_CONTROL_OFFSET_MV + pa_control_i + p + d;
        if (mv < PA_CONTROL_MV_MIN) mv = PA_CONTROL_MV_MIN;
        if (mv > PA_CONTROL_MV_MAX) mv = PA_CONTROL_MV_MAX;
        dac_ch2_write_mv((uint16_t)mv);

        last_control_loop = HAL_GetTick();
        pa_control_last_deviation = error;

        if (debug_update_requested) {
            debug_update_requested = false;

            debug_pa_loop(p, pa_control_i, d, error, rf_pa_read_vdet_mv(), mv);
        }
    }
}

#endif //USE_PA
