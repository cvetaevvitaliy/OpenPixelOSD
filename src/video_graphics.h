/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VIDEO_GRAPHICS_H
#define VIDEO_GRAPHICS_H

#if defined(HIGH_RAM)
#include <stdint.h>
#include "main.h"

#define VIDEO_WIDTH        (360U)
#define VIDEO_HEIGHT       (288U)
#define VIDEO_BPP          (2U)  // bits per pixel
#define VIDEO_BYTES_PER_LINE ((VIDEO_WIDTH * VIDEO_BPP) / 8U)

void video_graphics_init(void);
void video_graphics_clear(px_t color);
void video_draw_pixel(uint16_t x, uint16_t y, px_t color);

// Only for test
void video_draw_chessboard_test(void);
void video_draw_3d_cube(float size, float fov, float viewer_distance,
                        float angle_x, float angle_y, float angle_z,
                        int16_t origin_x, int16_t origin_y, px_t color);


#endif
#endif //VIDEO_GRAPHICS_H
