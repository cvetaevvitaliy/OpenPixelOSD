/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VIDEO_OVERLAY_H
#define VIDEO_OVERLAY_H

#include <stdint.h>

typedef enum {
    VIDEO_INPUT_1 = 0, // PA3
    VIDEO_INPUT_2 = 1  // PA7
} video_input_t;

void video_overlay_init(void);
// Call after video_overlay_init(). Switching preserves the OSD canvas and
// reacquires field sync on PA3 (input 1) or PA7 (input 2).
void set_video_input(video_input_t input);
video_input_t get_video_input(void);
void video_sync_loop(void);

// Read-only hardware diagnostics, also accessible to a debugger.
typedef struct {
    uint32_t captures;
    uint32_t full_lines;
    uint32_t fields;
    uint32_t armed_lines;
    uint32_t completed_lines;
    uint32_t dma_errors;
    uint32_t last_capture_ticks;
    uint32_t last_field_ms;
    uint16_t threshold_mv;
    uint8_t input;
    uint8_t locked;
} video_diagnostics_t;
void video_get_diagnostics(video_diagnostics_t *result);

#endif //VIDEO_OVERLAY_H
