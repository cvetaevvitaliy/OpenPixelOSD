/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VIDEO_OVERLAY_H
#define VIDEO_OVERLAY_H

#include "main.h"
#include <stdbool.h>

extern bool new_field;

void video_overlay_init(void);


#define LINE_BUF_SZ         (PIXELS_PER_LINE+2)

#if defined(USE_GRAPHICS)
#define DMA_DOUBLE_BUFFER   0
#else
#define DMA_DOUBLE_BUFFER   1
#endif

typedef enum {
  OFF,
  INTERNAL,
  EXTERNAL,
  AUTOMATIC
} syncMode_t;

typedef enum {
    SYNC_STATE_SEARCH,
    SYNC_STATE_FOUND,
    SYNC_STATE_EXTERNAL,
} syncState_t;

typedef enum {
  OSD_INIT,
  OSD_OFF,  
  OSD_MSP,
  OSD_MENU,
  OSD_EXIT_MENU
} osdState_e;

typedef enum {
    MODE_UNKNOWN,
    MODE_PAL,
    MODE_NTSC
} videoMode_t;

typedef struct {
    uint32_t  compInput;
    uint32_t  opampInput;
    uint8_t   gain;
} videoInput_t;

typedef struct {
    float     phase;
    uint16_t  luminance;
} colorMap_t;

extern osdState_e osdState;
extern bool show_logo;
extern bool show_test_pattern;
extern uint16_t video_levels[];
extern uint16_t sync_levels[];

void video_overlay_init(void);
void video_sync_loop(void);
void setSyncMode(syncMode_t mode);
void set_video_input(uint8_t input);

#endif //VIDEO_OVERLAY_H
