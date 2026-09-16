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
void set_video_input(video_input_t input);
video_input_t get_video_input(void);

#endif //VIDEO_OVERLAY_H
