/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"
#include "render/msp_displayport.h"
#include "system.h"
#include "hardware/usb.h"
#include "render/canvas_char.h"
#include "render/video_overlay.h"

#include <stdio.h>


#define LED_BLINK_INTERVAL 100 // milliseconds
#define LOGO_TIMEOUT_MS 4000 // 4 seconds

void led_blink(void);
void logo_timeout_check(void);



int startup(void){
    //Hardware Init:
    HAL_Init();
    SystemClock_Config();
    gpio_init();
    usb_init();
    dma_init();
    adc_init();

    //Video Init:
    video_overlay_init();
    msp_displayport_init();
    return 1;
}

int PAUSE(void){
    while (PAUSE_ON_FAILED_INIT){}
    return 0;
}

int main (void) 
{
    startup()
        ? printf("Init SUCCESS.\r\n")
        : (printf("Init FAILED.\r\n"), PAUSE());

    while (1)
    {
        #ifdef USE_MSP
        extern bool show_logo;
        msp_loop_process();
        msp_camera_switch_process();

        if (show_logo) {
            logo_timeout_check();
        }
        #endif

        #ifdef DEBUG_LED_BLINK
        led_blink();
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
