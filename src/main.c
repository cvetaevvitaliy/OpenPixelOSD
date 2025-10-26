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

#if defined(BUILD_VARIANT_VTX)
#include "rtc6705.h"
#include <rf_pa.h>
#endif
#include <stdio.h>

#if defined(HIGH_RAM)
#include "video_graphics.h"
extern bool new_field;
#endif

#define LED_BLINK_INTERVAL 100 // milliseconds
#define DEBUG_LOOP_INTERVAL 100 // milliseconds
#define LOGO_TIMEOUT_MS 4000 // 4 seconds

void led_blink(void);
void logo_timeout_check(void);

void debug_print_loop(void)
{
    static uint32_t last_tick = 0;

    if ((HAL_GetTick() - last_tick) >= DEBUG_LOOP_INTERVAL) {
        last_tick = HAL_GetTick();
        // Loop debug printf here
    }
}


int main (void)
{
    HAL_Init();
    SystemClock_Config();
    gpio_init();
    usb_init();
    dma_init();
    adc_init();

    video_overlay_init();
#if defined(HIGH_RAM)
    video_graphics_init();
    video_draw_text_system_font(FONT_SYSTEM_WIDTH * 2, VIDEO_HEIGHT - FONT_SYSTEM_HEIGHT, "WAITING MSP...");
    video_graphics_draw_complete();
#endif
    msp_displayport_init();

#if defined(BUILD_VARIANT_VTX)
    if(rtc6705_init()) {
        printf("rtc6705 detected\r\n");
        rtc6705_set_frequency(5880); // TODO: remove after implementing configuration saving to flash

        rf_pa_init();
        rf_pa_set_power_level(RF_PA_PWR_20mW);
    }
#endif

    while (1)
    {
        msp_loop_process();
        led_blink();
        debug_print_loop();
        logo_timeout_check();

#if 0 // TODO: remove later
// For test only - 3D cube animation
#if defined(HIGH_RAM)
        if (new_field == false) {
            video_draw_3d_cube_animation();
        }
#endif
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

void logo_timeout_check(void)
{
    static uint32_t boot_time = 0;
    static bool timeout_checked = false;
    extern bool show_logo;

    // Initialize boot time on first call
    if (boot_time == 0) {
        boot_time = HAL_GetTick();
    }

    // Check if LOGO_TIMEOUT_MS has elapsed, clear logo and version string if so
    if (!timeout_checked && (HAL_GetTick() - boot_time) >= LOGO_TIMEOUT_MS) {
        show_logo = false;
        // Clear the canvas to remove version string
        canvas_char_clean();
        canvas_char_draw_complete();
        timeout_checked = true;
    }
}
