/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdio.h>
#include "main.h"
#include "msp_displayport.h"
#include "system.h"
#include "usb.h"
#include "video_gen.h"
#include "video_overlay.h"

#define LED_BLINK_INTERVAL 100 // milliseconds

void led_blink(void);

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

