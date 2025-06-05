/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"
#include "msp_displayport.h"
#include "system.h"
#include "usb.h"
#include "video_gen.h"
#include "video_overlay.h"

#define LED_BLINK_INTERVAL 100 // milliseconds

void led_blink(void);

// For test only - 3D cube animation
#if defined(HIGH_RAM)
#include <math.h>
#include "video_graphics.h"

extern bool new_field;
extern uint8_t active_video_buffer;

void run_3d_cube_animation(void)
{
    static float angle_x = 0.0f;
    static float angle_y = 0.0f;
    static float angle_z = 0.0f;
    static float fov = 400.0f;
    static float fov_inc = 5.0f;

    video_graphics_clear(PX_TRANSPARENT);

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
#endif


int main (void)
{

    HAL_Init();
    SystemClock_Config();
    gpio_init();
    usb_init();
    dma_init();

    video_overlay_init();

    msp_displayport_init();

    while (1)
    {
        msp_loop_process();
        led_blink();

// For test only - 3D cube animation
#if defined(HIGH_RAM)
        if (new_field == false) {
            run_3d_cube_animation();
        }
#endif
    }
}

void led_blink(void)
{
    static uint32_t last_tick = 0;

    if ((HAL_GetTick() - last_tick) >= LED_BLINK_INTERVAL) {
        LED_STATE_GPIO_Port->ODR ^= LED_STATE_Pin;
        last_tick = HAL_GetTick();
    }
}

