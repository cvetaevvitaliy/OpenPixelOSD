/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <usb_device.h>
#include "main.h"
#include "system.h"
#include "video_gen.h"
#include "video_overlay.h"
#include "msp_displayport.h"


int main (void)
{

    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    MX_USB_Device_Init();
    DMA_Init();
    TIM7_Init();

    video_overlay_init();

    msp_displayport_init();

    while (1)
    {
        GPIOC->ODR ^= LL_GPIO_PIN_6;
        LL_mDelay(100);
    }
}
