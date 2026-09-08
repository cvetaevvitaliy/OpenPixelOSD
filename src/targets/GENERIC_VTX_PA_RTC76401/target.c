/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/GENERIC_VTX_PA_RTC76401/target.c — power table for the RTC76401
 * external PA board.
 *
 * RTC6705 register: PA5G_PW is not a gain ladder on this board. Each bit
 * independently gates a separate physical output pin (bit0=PAOUT1,
 * bit1=PAOUT2), standard RTC6705 architecture for boards that combine
 * both outputs for extra power. This board only routes PAOUT1 externally
 * (PAOUT2 is N/C), so only bit0 matters:
 *   RTC6705_PA_3dBm  (00) and RTC6705_PA_11dBm (10) -> PAOUT1 off (identical externally)
 *   RTC6705_PA_7dBm  (01) and RTC6705_PA_13dBm (11) -> PAOUT1 on  (identical externally)
 * Only two RTC6705-alone states are available here, not four.
 *
 * DAC bias (Q2/SSM3J56MFV, PAOUT1 injection): the off->on transition
 * happens within roughly 2800-3200mV; below ~2400mV it's fully saturated
 * and further gate drive does nothing more. The useful trim range is
 * that ~400mV window, not the full 0-3300mV span.
 *
 * detector[] targets for the four boost-enabled levels (50/100/150/200mW)
 * were confirmed reachable via the closed loop: 50/100/150mW converge to
 * 0 error, 200mW converges with a small residual (~-55mV, integral-clamp
 * limited) rather than diverging. Still not power-meter verified mW
 * output -- confirm each against a real power meter before trusting the
 * mW label.
 *
 * Level NUMBERS are 1-based (see vtx_power_levels.h) but this array is
 * plain 0-based -- level N is entry [N-1] below, there is no reserved
 * slot 0. This is the DEFAULTS table (g_vtx_power_level_defaults[]),
 * copied into the RAM-backed g_vtx_power_levels[] at boot by
 * vtx_power_levels_init() and then overlaid with any EEPROM data (see
 * vtx_power_levels.c) -- edit this file to change what a freshly-
 * flashed/never-configured board starts with, not what an
 * already-configured one uses. calibration[]/detector[] here are only
 * VTX_CAL_FREQ_POINTS_MAX-sized arrays because that's this file's own
 * compile-time ceiling on breakpoints -- only the first
 * g_vtx_cal_freq_default_count entries below are meaningful.
 */
#include "main.h"
#include "vtx_power_levels.h"

const vtx_power_level_t g_vtx_power_level_defaults[] = {
    { 0,   RTC6705_PA_3dBm,  false, {3200,3200,3200,3200,3200,3200,3200,3200,3200}, {0,0,0,0,0,0,0,0,0} }, // level 1: PAOUT1 off, Q2 off -- matches pit-mode baseline (~310mA)
    { 0,   RTC6705_PA_3dBm,  false, {2400,2400,2400,2400,2400,2400,2400,2400,2400}, {0,0,0,0,0,0,0,0,0} }, // level 2: PAOUT1 off, Q2 off -- with a more DC bias
    { 0,   RTC6705_PA_7dBm,  false, {3200,3200,3200,3200,3200,3200,3200,3200,3200}, {0,0,0,0,0,0,0,0,0} }, // level 3: PAOUT1 on, Q2 off -- RTC6705's own drive alone, no boost
    { 0,   RTC6705_PA_7dBm,  false, {2400,2400,2400,2400,2400,2400,2400,2400,2400}, {0,0,0,0,0,0,0,0,0} }, // level 4: PAOUT1 on, Q2 off -- with more DC bias
    { 10,  RTC6705_PA_3dBm,  true,  {2790,2790,2790,2790,2790,2790,2790,2790,2790}, {138,138,138,138,138,138,138,138,138} }, // level 5: boost on
    { 25,  RTC6705_PA_3dBm,  true,  {2775,2775,2775,2775,2775,2775,2775,2775,2775}, {200,200,200,200,200,200,200,200,200} }, // level 6: boost on
    { 50,  RTC6705_PA_7dBm,  true,  {2750,2750,2750,2750,2750,2750,2750,2750,2750}, {260,260,260,260,260,260,260,260,260} }, // level 7: boost on
    { 100, RTC6705_PA_7dBm,  true,  {2710,2710,2710,2710,2710,2710,2710,2710,2710}, {300,300,300,300,300,300,300,300,300} }, // level 8: boost on
};

const uint8_t g_vtx_power_level_default_count = sizeof(g_vtx_power_level_defaults) / sizeof(g_vtx_power_level_defaults[0]);

const uint16_t g_vtx_cal_freq_defaults_mhz[] = {5658,5695,5760,5800,5840,5905,5945};
const uint8_t g_vtx_cal_freq_default_count = sizeof(g_vtx_cal_freq_defaults_mhz) / sizeof(g_vtx_cal_freq_defaults_mhz[0]);
