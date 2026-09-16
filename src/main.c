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
#include <string.h>

#include <stdio.h>


#define LED_BLINK_INTERVAL 3000 // milliseconds

void led_blink(void);



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
        video_sync_loop();
        #ifdef USE_MSP
        // Receive DisplayPort even while the startup logo is visible.
        msp_loop_process();
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
        set_video_input(get_video_input() == VIDEO_INPUT_1 ? VIDEO_INPUT_2 : VIDEO_INPUT_1);
    }
}
