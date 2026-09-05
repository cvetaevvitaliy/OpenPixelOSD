/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "video_graphics.h"
#include "main.h"

#if defined(USE_GRAPHICS)
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "canvas_char.h"
#include "video_overlay.h"
#include "fonts/font_betaflight.h"

#include "fonts/font_system.h"
#include <stdarg.h>
#include <stdio.h>
#include "logo/logo.h"

/* Tunable temporary buffer size for formatted text */
#ifndef VIDEO_TEXT_TMP_MAX
#define VIDEO_TEXT_TMP_MAX 256
#endif

#define LOGO_OFFSET_Y       (25)

uint8_t video_frame_buffer[2][VIDEO_HEIGHT][VIDEO_BYTES_PER_LINE];
CCMRAM_DATA uint8_t active_video_buffer = 0;
CCMRAM_DATA uint8_t paint_video_buffer = 1;

void video_graphics_init(void)
{
    for(int active_video_buffer = 0; active_video_buffer < 2; active_video_buffer++) {
        video_graphics_clear_draw_buff(PX_TRANSPARENT);
    }
    active_video_buffer = 0;
    paint_video_buffer = 1;
}

EXEC_RAM void video_graphics_clear_draw_buff(px_t color)
{
  #if VIDEO_BPP == 3
  uint8_t pixel_byte[3];
  pixel_byte[0] = ((color & 0x7) << 5) | ((color & 0x7) << 2) |  ((color & 0x7) >> 1);
  pixel_byte[1] = ((color & 0x7) << 7) | ((color & 0x7) << 4) |  ((color & 0x7) << 1) |  ((color & 0x7) >> 2);
  pixel_byte[2] = ((color & 0x7) << 6) | ((color & 0x7) << 3) |  ((color & 0x7));

  uint8_t* buffer = (uint8_t*)pixel_byte;

  paint_video_buffer = 1 - active_video_buffer;

  for(uint16_t y = 0 ; y < VIDEO_HEIGHT; y++) {
    for(uint16_t x = 0; x< VIDEO_BYTES_PER_LINE; x++ ) {
      video_frame_buffer[paint_video_buffer][y][x] = buffer[x % 3];
    }
  }
  #else
  uint8_t pixel_byte = ((color & 0x3) << 6) |
                         ((color & 0x3) << 4) |
                         ((color & 0x3) << 2) |
                         ((color & 0x3) << 0);

  paint_video_buffer = 1 - active_video_buffer;

  memset(video_frame_buffer[paint_video_buffer], pixel_byte, sizeof(video_frame_buffer[paint_video_buffer]));
  #endif
}


void video_draw_pixel(uint16_t x, uint16_t y, px_t color)
{
    if (x >= VIDEO_WIDTH || y >= VIDEO_HEIGHT) return;

    const uint8_t mask = (1 << VIDEO_BPP) - 1;

    uint32_t bit_index  = x * VIDEO_BPP;
    uint32_t byte_index = bit_index >> 3;
    uint8_t  shift      = 7 - (bit_index & 7);

    uint8_t *byte = &video_frame_buffer[paint_video_buffer][y][byte_index];

    #if VIDEO_BPP == 3
    if (shift + 1U >= VIDEO_BPP) {
    #endif 
        uint8_t s = shift + 1 - VIDEO_BPP;
        *byte &= ~(mask << s);
        *byte |= ((color & mask) << s);
        return;
    #if VIDEO_BPP == 3
    } else if (shift == 1) {
      *byte &= ~0x03;
      *byte |=  (color>>1) & 0x03;

      uint8_t *byte2 = byte + 1;
      *byte2 &= ~(0x01<<7);
      *byte2 |=  (color & 0x01)<<7;
    } else {
      *byte &= ~0x01;
      *byte |=  (color>>2) & 0x01;

      uint8_t *byte2 = byte + 1;
      *byte2 &= ~(0x03<<6);
      *byte2 |=  (color & 0x03)<<6;
    }
    #endif
}


EXEC_RAM static inline void vg_put_px2(uint16_t x, uint16_t y, uint8_t v2)
{
    if (x >= VIDEO_WIDTH || y >= VIDEO_HEIGHT) return;
    uint8_t *line = &video_frame_buffer[paint_video_buffer][y][0];
    uint16_t byte = x >> 2;                 /* 4 pixels per byte */
    uint8_t  sub  = x & 0x3;                /* 0..3 */
    uint8_t  shift = (uint8_t)((3u - sub) * 2u);
    uint8_t  mask  = (uint8_t)(0x3u << shift);
    line[byte] = (uint8_t)((line[byte] & ~mask) | ((v2 & 0x3u) << shift));
}

static const uint8_t colorMatrix[4][16] = { {0,1,2,3,4,5,6,7,2,2,2,2,2,2,2,2},
                                            {0,1,2,3,4,5,6,7,2,2,2,2,2,2,2,4},
                                            {0,1,2,3,4,5,6,7,2,2,2,2,2,2,2,7},
                                            {0,1,2,3,4,5,6,7,2,2,2,2,2,2,2,5}
                                          };

EXEC_RAM void video_draw_char_at(char ch, uint16_t x, uint16_t y, uint8_t font)
{
    UNUSED(font);
    //uint8_t fontColor[] = {PX_WHITE, PX_GREEN, PX_YELLOW, PX_RED};
    const font_t *glyph = &font_data[(uint8_t)ch * FONT_STRIDE / sizeof(font_t)];

    for (uint16_t glyph_row = 0; glyph_row < FONT_HEIGHT; glyph_row++) {

        uint16_t dst_y = y + glyph_row;
        if (dst_y >= VIDEO_HEIGHT) continue;

        uint32_t row_off = (uint32_t)glyph_row * (uint32_t)BYTES_PER_ROW / sizeof(font_t);

        for (uint16_t gx = 0; gx < FONT_WIDTH; gx++) {

            uint16_t dst_x = x + gx;
            if (dst_x >= VIDEO_WIDTH) continue;

            uint32_t bitpos     = (uint32_t)gx * (uint32_t)FONT_BPP;
            uint32_t byte_index = row_off + (gx >> 2);
            uint32_t bit_offset = bitpos & (FONT_BPP * 4 - 1);

            font_t raw_byte = glyph[byte_index];

            uint8_t px2 = (font_t)((raw_byte >> (FONT_BPP * 3 - bit_offset)) & ((1<<FONT_BPP) - 1));

            video_draw_pixel(dst_x, dst_y, (px_t)colorMatrix[font][px2]);
        }
    }
}


void video_graphics_draw_logo()
{
  #ifdef USE_COLOR
  uint8_t colorMatrix[] = {6,1,7,5};
  #else
  uint8_t colorMatrix[] = {0,1,2,3};
  #endif

  for (uint16_t y = 0; y < LOGO_HEIGHT; y++) {
    const uint8_t *logo_line_ptr = &logo_data[y * LOGO_ROW_BYTES];

    uint16_t logoOffsetX = (PIXELS_PER_LINE -LOGO_WIDTH) / 2;

    for (uint16_t x = 0; x < LOGO_WIDTH>>2; x++) {
      uint8_t byte = logo_line_ptr[x];
      uint8_t pixel;
  
      pixel = (byte >> 6) & 0x3;
      video_draw_pixel((uint16_t)(logoOffsetX + (x<<2)), LOGO_OFFSET_Y + y, (px_t)colorMatrix[pixel]);

      pixel = (byte >> 4) & 0x3;
      video_draw_pixel((uint16_t)(logoOffsetX + (x<<2) + 1), LOGO_OFFSET_Y + y, (px_t)colorMatrix[pixel]);

      pixel = (byte >> 2) & 0x3;
      video_draw_pixel((uint16_t)(logoOffsetX + (x<<2) + 2), LOGO_OFFSET_Y + y, (px_t)colorMatrix[pixel]);

      pixel = byte & 0x3;
      video_draw_pixel((uint16_t)(logoOffsetX + (x<<2) + 3), LOGO_OFFSET_Y + y, (px_t)colorMatrix[pixel]);
    }
  }
}

void video_graphics_draw_test_pattern() {
  video_draw_rectangle(50,  25, 75,  75, 0);
  video_draw_rectangle(75,  25, 100, 75, 2);
  video_draw_rectangle(100, 25, 125, 75, 3);
  video_draw_rectangle(125, 25, 150, 75, 4);
  video_draw_rectangle(150, 25, 175, 75, 5);
  video_draw_rectangle(175, 25, 200, 75, 6);
  video_draw_rectangle(200, 25, 225, 75, 7);
  //video_draw_rectangle(225, 25, 250, 100, 3);

  video_draw_rectangle(10,  150, 350,  200, PX_BLACK);
  video_draw_rectangle(10,  225, 350,  250, PX_WHITE);
  for(uint8_t x=0; x<5; x++) {
    video_draw_rectangle(30  + x * 10, 125, 35  + x * 10,  250, PX_WHITE);
    video_draw_rectangle(80  + x * 10, 125, 85  + x * 10,  150, PX_WHITE);
    video_draw_rectangle(130 + x * 10, 125, 135 + x * 10,  150, PX_WHITE);
    video_draw_rectangle(180 + x * 10, 125, 185 + x * 10,  150, PX_WHITE);
    video_draw_rectangle(230 + x * 10, 125, 235 + x * 10,  150, PX_WHITE);
    video_draw_rectangle(80  + x * 10, 150, 85  + x * 10,  250, PX_YELLOW);
    video_draw_rectangle(130 + x * 10, 150, 135 + x * 10,  250, PX_GREEN);
    video_draw_rectangle(180 + x * 10, 150, 185 + x * 10,  250, PX_RED);
    video_draw_rectangle(230 + x * 10, 150, 235 + x * 10,  250, PX_BLUE);
    
  }
}

EXEC_RAM void video_graphics_draw_complete(void)
{
    if (show_test_pattern) {
      video_graphics_draw_test_pattern();
    } else if (show_logo) {
      video_graphics_draw_logo();
    }
    active_video_buffer = paint_video_buffer;
    //video_graphics_clear_draw_buff(PX_TRANSPARENT);
}

EXEC_RAM bool bit_at(const uint8_t *glyph, uint8_t gx, uint8_t gy, uint8_t BPR) {
    const uint8_t *row = glyph + (uint32_t)gy * BPR;
    /* MSB-first within each byte */
    uint8_t byte = row[gx >> 3];
    uint8_t shift = (uint8_t)(7 - (gx & 7));
    return ((byte >> shift) & 1u) != 0;
}

// Render a single 8-bit code (0..255) at top-left (x,y).
// Transparent: skip zero bits. Foreground = PX_WHITE.
// Adds a 1px black outline (4-neighborhood) without overwriting white strokes.
EXEC_RAM void video_draw_char_1bpp(uint16_t x, uint16_t y, uint8_t c)
{
    const font1bpp_t *f = font_system_get();
    if (!f || !f->data) return;

    const uint8_t *glyph = f->data + (uint32_t)c * f->stride;
    const uint8_t W = f->glyph_w;
    const uint8_t H = f->glyph_h;
    const uint8_t BPR = f->bytes_per_row; /* bytes per glyph row (MSB-first) */

    /* ---- Pass 1: draw black outline on 4-neighbors around set pixels ---- */
    for (uint8_t gy = 0; gy < H; gy++) {
        for (uint8_t gx = 0; gx < W; gx++) {
            if (!bit_at(glyph, gx, gy, BPR)) continue; /* only around set pixels */

            const int dx0 = (int)x + gx;
            const int dy0 = (int)y + gy;

            /* 4-neighborhood offsets */
            static const int8_t n4[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
            for (int k = 0; k < 4; k++) {
                int ngx = gx + n4[k][0];
                int ngy = gy + n4[k][1];

                /* Draw outline if neighbor is outside glyph bounds OR is an empty pixel */
                bool draw_outline = false;
                if (ngx < 0 || ngy < 0 || ngx >= (int)W || ngy >= (int)H) {
                    draw_outline = true;
                } else if (!bit_at(glyph, (uint8_t)ngx, (uint8_t)ngy, BPR)) {
                    draw_outline = true;
                }

                if (draw_outline) {
                    int tx = dx0 + n4[k][0];
                    int ty = dy0 + n4[k][1];
                    if (tx >= 0 && ty >= 0 && tx < (int)VIDEO_WIDTH && ty < (int)VIDEO_HEIGHT) {
                        video_draw_pixel((uint16_t)tx, (uint16_t)ty, PX_BLACK);
                    }
                }
            }
        }
    }

    /* ---- Pass 2: draw white glyph pixels (overwrite any black under-stroke) ---- */
    for (uint8_t gy = 0; gy < H; gy++) {
        uint16_t dy = (uint16_t)((int)y + gy);
        if (dy >= VIDEO_HEIGHT) continue;

        const uint8_t *row = glyph + (uint32_t)gy * BPR;
        uint16_t dx = x;

        uint16_t remaining = W;
        const uint8_t *p = row;

        while (remaining > 0) {
            uint8_t bits = *p++;
            uint8_t valid = (remaining >= 8) ? 8 : (uint8_t)remaining;

            /* MSB-first: bit7 -> leftmost pixel */
            for (int b = 7; b >= 8 - (int)valid; b--) {
                if ((bits >> b) & 1u) {
                    if (dx < VIDEO_WIDTH) {
                        video_draw_pixel(dx, dy, PX_WHITE);
                    }
                }
                dx++;
            }
            remaining -= valid;
        }
    }
}

// Render string (bytes are direct glyph codes 0..255) at (x,y).
// Returns the X cursor after rendering (monospace advance = glyph_w).
EXEC_RAM uint16_t video_draw_text_system_font(uint16_t x, uint16_t y, const char *s)
{
    const font1bpp_t *f = font_system_get();
    if (!f || !s) return x;
    uint16_t cursor = x;
    while (*s) {
        video_draw_char_1bpp(cursor, y, *s);
        cursor = (uint16_t)(cursor + f->glyph_w);
        s++;
    }
    return cursor;
}

/* Render formatted text with the system 1bpp font at (x, y).
 * Returns the current X cursor after rendering the last line.
 * Notes:
 *  - bytes are treated as direct glyph codes 0..255 (no UTF-8 decode)
 *  - '\n' moves to the next line (y += glyph_h) and resets X to start_x
 *  - string is truncated to VIDEO_TEXT_TMP_MAX-1 if longer
 */
EXEC_RAM uint16_t video_draw_text_system_font_fmt(uint16_t x, uint16_t y, const char *fmt, ...)
{
    const font1bpp_t *f = font_system_get();
    if (!f || !fmt) return x;

    char buf[VIDEO_TEXT_TMP_MAX];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n <= 0) return x;  /* nothing to draw or formatting error */

    /* Clamp to buffer length (vsnprintf already NUL-terminates) */
    const unsigned char *p = (const unsigned char *)buf;
    const uint16_t start_x = x;

    while (*p) {
        unsigned char c = *p++;

        if (c == '\n') {
            /* new line: move down by glyph height, reset X */
            y = (uint16_t)(y + f->glyph_h);
            x = start_x;
            continue;
        }

        /* draw single glyph 0..255 at (x,y) */
        video_draw_char_1bpp(x, y, c);
        x = (uint16_t)(x + f->glyph_w);
    }
    return x;
}


/******** Only for test */

#define NUM_VERTICES 8
#define NUM_EDGES    12

void video_draw_chessboard_test(void)
{
    const uint16_t half_width = VIDEO_WIDTH / 2;
    const uint16_t block_size = 16;
    const px_t colors[2] = {PX_BLACK, PX_WHITE};

    for (uint16_t y = 0; y < VIDEO_HEIGHT; y++) {
        for (uint16_t x = 0; x < half_width; x++) {
            uint8_t block_x = x / block_size;
            uint8_t block_y = y / block_size;
            px_t color = colors[(block_x + block_y) % 2];
            video_draw_pixel(x, y, color);
        }
    }
}


typedef struct {
    float x;
    float y;
    float z;
} vec3_t;

typedef struct {
    int16_t x;
    int16_t y;
} point2d_t;

point2d_t project(vec3_t point, float fov, float viewer_distance, int16_t origin_x, int16_t origin_y)
{
    float factor = fov / (viewer_distance + point.z);
    point2d_t p;
    p.x = (int16_t)(point.x * factor + origin_x);
    p.y = (int16_t)(point.y * factor + origin_y);
    return p;
}

void video_draw_line(int x0, int y0, int x1, int y1, px_t color)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        video_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void video_draw_rectangle(int x0, int y0, int x1, int y1, px_t color)
{
    for (uint16_t y = y0; y<=y1; y++) {
      for (uint16_t x = x0; x<=x1; x++) {
        video_draw_pixel(x, y, color);
      }
    }
}

static const vec3_t cube_vertices[NUM_VERTICES] = {
    { -1, -1, -1 },
    {  1, -1, -1 },
    {  1,  1, -1 },
    { -1,  1, -1 },
    { -1, -1,  1 },
    {  1, -1,  1 },
    {  1,  1,  1 },
    { -1,  1,  1 }
};

static const uint8_t cube_edges[NUM_EDGES][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, // front
    {4, 5}, {5, 6}, {6, 7}, {7, 4}, // back
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  // squash
};

void video_draw_3d_cube(float size, float fov, float viewer_distance,
                        float angle_x, float angle_y, float angle_z,
                        int16_t origin_x, int16_t origin_y, px_t color)
{
    float cos_x = cosf(angle_x);
    float sin_x = sinf(angle_x);
    float cos_y = cosf(angle_y);
    float sin_y = sinf(angle_y);
    float cos_z = cosf(angle_z);
    float sin_z = sinf(angle_z);

    //vec3_t rotated_vertices[NUM_VERTICES];
    point2d_t projected[NUM_VERTICES];

    for (int i = 0; i < NUM_VERTICES; i++) {
        vec3_t v = cube_vertices[i];

        v.x *= size;
        v.y *= size;
        v.z *= size;

        // Rotation X
        float y1 = v.y * cos_x - v.z * sin_x;
        float z1 = v.y * sin_x + v.z * cos_x;

        // Rotation Y
        float x2 = v.x * cos_y + z1 * sin_y;
        float z2 = -v.x * sin_y + z1 * cos_y;

        // Rotation Z
        float x3 = x2 * cos_z - y1 * sin_z;
        float y3 = x2 * sin_z + y1 * cos_z;

        vec3_t rotated = { x3, y3, z2 + 5.0f }; // shift Z forward
        //rotated_vertices[i] = rotated;

        projected[i] = project(rotated, fov, viewer_distance, origin_x, origin_y);
    }

    // Draw edges
    for (int i = 0; i < NUM_EDGES; i++) {
        uint8_t start = cube_edges[i][0];
        uint8_t end   = cube_edges[i][1];
        video_draw_line(projected[start].x, projected[start].y, projected[end].x, projected[end].y, color);
    }
}

void video_draw_3d_cube_animation(void)
{
    static float angle_x = 0.0f;
    static float angle_y = 0.0f;
    static float angle_z = 0.0f;
    static float fov = 400.0f;
    static float fov_inc = 5.0f;

    video_graphics_clear_draw_buff(PX_TRANSPARENT);

    video_draw_3d_cube(80.0f, 110.0f, fov, angle_x, angle_y, angle_z,
                       180, 220, PX_WHITE);

    video_draw_3d_cube(50.0f, 100.0f, fov, angle_x, angle_y, angle_z, 180, 220, PX_WHITE);

    active_video_buffer ^= 1;

    angle_x += 0.01f;
    angle_y += 0.013f;
    angle_z += 0.017f;
    fov -= fov_inc;

    if (fov < 200.0f) fov_inc = -5.0f;
    if (fov > 400.0f) fov_inc = 5.0f;

    if (angle_x >= 2 * M_PI) angle_x -= 2 * M_PI;
    if (angle_y >= 2 * M_PI) angle_y -= 2 * M_PI;
    if (angle_z >= 2 * M_PI) angle_z -= 2 * M_PI;
}
#else
// Empty file placeholder
void empty_unit(void) {}
#endif
