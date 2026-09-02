/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VTX_MSP_H
#define VTX_MSP_H
#include <stdint.h>
#include <stdbool.h>

                              /* VTX bands table (letter + 8-char name + 8 channel freqs (MHz)).
                              * These are standard bands used in Betaflight and iNav.
                              * You can add custom bands here if needed. */
#define BAND_TABLE_DEFAULT    {   /* Band A (Boscam A) */                           \
                                  { 'A', { 'B','O','S','C','A','M',' ','A' },       \
                                    { 5865,5845,5825,5805,5785,5765,5745,5725 } },  \
                                  /* Band B (Boscam B) */                           \
                                  { 'B', { 'B','O','S','C','A','M',' ','B' },       \
                                    { 5733,5752,5771,5790,5809,5828,5847,5866 } },  \
                                  /* Band E */                                      \
                                  { 'E', { 'B','A','N','D',' ','E',' ',' ' },       \
                                    { 5705,5685,5665,5645,5885,5905,5925,5945 } },  \
                                  /* Band F (FatShark) */                           \
                                  { 'F', { 'F','A','T','S','H','A','R','K' },       \
                                    { 5740,5760,5780,5800,5820,5840,5860,5880 } },  \
                                  /* Band R (Raceband) */                           \
                                  { 'R', { 'R','A','C','E','B','A','N','D' },       \
                                    { 5658,5695,5732,5769,5806,5843,5880,5917 } },  \
                              }

/* Optional helpers to query current state (for OSD, logs, etc.) */
typedef struct {
  uint8_t band;        // 1..5 (A/B/E/F/R), 0 if using frequency
  uint8_t channel;     // 1..8
  uint16_t frequency;  // MHz; if nonzero, overrides band/channel on SET
  uint8_t power;       // power index (0..N-1)
  uint8_t pitmode;     // 0/1
  uint8_t vtx_table_available;
  uint8_t configSet;
} vtx_config_t;

/* ---- VTX bands table: letter + 8-char name + 8 channel freqs (MHz) ---- */
#define VTX_CHANNEL_COUNT    8
#define VTX_CH_LABEL_COUNT   8
#define VTX_IS_FACTORY_BAND  1

typedef struct {
    char letter;                            /* 'A','B','E','F','R' */
    uint8_t band_name[VTX_CH_LABEL_COUNT];  /* shown in BF “Name”, exactly 8 bytes */
    uint16_t freq[VTX_CHANNEL_COUNT];       /* ch1..ch8, MHz */
} vtx_band_t;

const vtx_config_t* vtx_get_config(void);
const char* vtx_get_band_name(uint8_t band);
uint8_t vtx_get_band_count(void);
uint16_t vtx_get_power_mw(void);
uint16_t vtx_get_frequency(uint8_t band, uint8_t channel);
void vtx_set_pitmode(uint8_t pitmode);
void vtx_set_band_channel(int8_t band, uint8_t channel);
void vtx_set_power(int8_t power);

bool vtx_msp_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);
void vtx_msp_request_config(uint8_t owner);

void vtx_msp_clear_table_and_set_defaults(uint8_t owner);
void vtx_msp_push_power_table(uint8_t owner);
void vtx_msp_push_band_table(uint8_t owner);
void vtx_msp_push_calibration_table(uint8_t owner);
#if defined(USE_PA)
void vtx_msp_set_calibration_table(uint8_t owner, const uint8_t *payload, uint16_t data_size);
void vtx_msp_push_calibration(uint8_t owner);
void vtx_msp_set_calibration(uint8_t owner, const uint8_t *payload, uint16_t data_size);
#endif
void vtx_msp_eeprom_write(uint8_t owner);

#endif //VTX_MSP_H
