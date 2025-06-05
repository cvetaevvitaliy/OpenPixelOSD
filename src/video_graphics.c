/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "video_graphics.h"

#if defined(HIGH_RAM)
#include <stdlib.h>
#include <math.h>
#include <string.h>

uint8_t video_frame_buffer[2][VIDEO_HEIGHT][VIDEO_BYTES_PER_LINE];
CCMRAM_DATA uint8_t active_video_buffer = 0;

void video_graphics_init(void)
{
    active_video_buffer = 0;
    video_graphics_clear(PX_TRANSPARENT);
}

void video_graphics_clear(px_t color)
{
    uint8_t pixel_byte = ((color & 0x3) << 6) |
                         ((color & 0x3) << 4) |
                         ((color & 0x3) << 2) |
                         ((color & 0x3) << 0);

    memset(video_frame_buffer, pixel_byte, sizeof(video_frame_buffer));
}

void video_draw_pixel(uint16_t x, uint16_t y, px_t color)
{
    if (x >= VIDEO_WIDTH || y >= VIDEO_HEIGHT) return;

    uint8_t *byte = &video_frame_buffer[active_video_buffer][y][x >> 2];
    uint8_t shift = 6 - ((x & 0x3) << 1);

    *byte &= ~(0x3 << shift);
    *byte |= ((color & 0x3) << shift);
}

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

// Only for test

#include <math.h>

#define NUM_VERTICES 8
#define NUM_EDGES    12

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

#endif
