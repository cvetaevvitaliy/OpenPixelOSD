/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef VIDEO_GRAPHICS_H
#define VIDEO_GRAPHICS_H

#include "main.h"

#if defined(USE_GRAPHICS)
#include <fonts/font_system.h>
#include <stdint.h>

#define VIDEO_WIDTH        (COLUMN_SIZE * 12U)
#define VIDEO_HEIGHT       (ROW_SIZE * 18U)
#ifdef USE_COLOR
#define VIDEO_BPP          (3U)
#else
#define VIDEO_BPP          (2U) 
#endif
#define VIDEO_BYTES_PER_LINE ((VIDEO_WIDTH * VIDEO_BPP) / 8U)

extern uint8_t active_video_buffer;
extern uint8_t video_frame_buffer[2][VIDEO_HEIGHT][VIDEO_BYTES_PER_LINE];

void video_graphics_init(void);
void video_draw_pixel(uint16_t x, uint16_t y, px_t color);
void video_draw_line(int x0, int y0, int x1, int y1, px_t color);
void video_draw_rectangle(int x0, int y0, int x1, int y1, px_t color);

void video_render_canvas_from_map(void);
void video_graphics_draw_complete(void);
void video_graphics_clear_draw_buff(px_t color);
void video_draw_char_at(char ch, uint16_t x, uint16_t y, uint8_t font);

uint16_t video_draw_text_system_font(uint16_t x, uint16_t y, const char *s);
uint16_t video_draw_text_system_font_fmt(uint16_t x, uint16_t y, const char *fmt, ...);

// Only for test
void video_draw_chessboard_test(void);
void video_draw_3d_cube(float size, float fov, float viewer_distance,
                        float angle_x, float angle_y, float angle_z,
                        int16_t origin_x, int16_t origin_y, px_t color);
void video_draw_3d_cube_animation(void);


#endif
#endif //VIDEO_GRAPHICS_H
