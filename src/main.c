/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"
#include "msp.h"
#include "msp_displayport.h"
#include "system.h"
#include "usb.h"
#include "video_gen.h"
#include "video_overlay.h"
#include "canvas_char.h"

#include "flash.h"
#include "led.h"
#include "settings.h"
#include "settingsMenu.h"
#if defined(USE_VTX)
#include "rtc6705.h"
#include "rf_pa.h"
#include "vtx_msp.h"
#endif
#if defined(USE_SWO)
#include "dbgu.h"
#endif
#include <stdio.h>

#if defined(USE_GRAPHICS)
#include "video_graphics.h"
#endif

#define LED_BLINK_INTERVAL 100 // milliseconds
#define DEBUG_LOOP_INTERVAL 1000 // milliseconds
#define LOGO_TIMEOUT_MS 4000 // 4 seconds

void led_blink(void);
void logo_timeout_check(void);

extern volatile uint16_t sync_voltage;
extern uint16_t sync_voltage_low;
#if defined(USE_VTX)
extern double rf_detector;
#endif

#define BOOTLOADER_ADDR  0x1FFF0000U
typedef void (*funcPtr)(void);

void check_bootloader(void) 
{
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  LL_PWR_EnableBkUpAccess();

  if(LL_RTC_BKP_GetRegister(RTC, LL_RTC_BKP_DR1) == MSP_REBOOT_BOOTLOADER_ROM) {
    LL_RTC_BKP_SetRegister(RTC, LL_RTC_BKP_DR1, MSP_REBOOT_FIRMWARE);
    __disable_irq();

    // 2. SysTick stoppen
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    LL_RCC_DeInit();
    LL_PWR_DisableBkUpAccess();
    LL_SYSCFG_SetRemapMemory(LL_SYSCFG_REMAP_SYSTEMFLASH);


    SCB->VTOR = BOOTLOADER_ADDR;

    uint32_t bootloaderStack = *(uint32_t *)(BOOTLOADER_ADDR);
    __set_MSP(bootloaderStack);

    uint32_t bootloaderEntryAddr = *(uint32_t *)(BOOTLOADER_ADDR + 4);
    funcPtr bootloaderEntry = (funcPtr)bootloaderEntryAddr;

    bootloaderEntry();
  }
}

void debug_print_loop(void)
{
    static uint32_t last_tick = 0;

    if ((HAL_GetTick() - last_tick) >= DEBUG_LOOP_INTERVAL) {
        last_tick = HAL_GetTick();
        TRACE_INFO("sync V:%i bl: %i sync low:%i \n",
          sync_voltage, 
          (uint16_t)DAC12BIT_TO_MV(sync_levels[1] / VIDEO_TOTAL_GAIN), 
          sync_voltage_low
          ); // Loop debug printf here
    }
}

#if defined(BUILD_VARIANT_BLINKY)
int main (void)
{
    HAL_Init();
    SystemClock_Config();
    gpio_init();

    while (1) {
        led_blink();
    }
}
#endif

#if !defined(BUILD_VARIANT_BLINKY)
int main (void)
{
    check_bootloader();
    HAL_Init();
    SystemClock_Config();
#ifdef USE_SWO
    SWO_Init();
    TRACE_INFO_WP("\n");
    TRACE_INFO("OpenPixelOSD\n");
    TRACE_INFO("Compiled: %s %s --\n", __DATE__, __TIME__);
#endif
    gpio_init();

    usb_init();
    dma_init();
    led_init();
    adc_init();
    flash_init();              // before anything reads/writes EEPROM
    settings_load();
#if defined(USE_PA)
    vtx_power_levels_init();   // after flash_init(), before rf_pa_init()/first MSP config
#endif

    video_overlay_init();

#if defined(USE_GRAPHICS)
    video_graphics_init();
    video_draw_text_system_font(FONT_SYSTEM_WIDTH * 2, VIDEO_HEIGHT - FONT_SYSTEM_HEIGHT, "WAITING MSP...");
    video_graphics_draw_complete();
#endif
    msp_init();

#if defined(USE_VTX)
    if(rtc6705_init()) {
        printf("rtc6705 detected\r\n");

#if defined(USE_PA)
        rf_pa_init(); // must run before rtc6705_set_frequency(): that call
                      // gates rf_pa_disable()/rf_pa_enable() around retuning,
                      // which need PA_ON_Pin already in output mode and
                      // DAC1 ch2 already enabled.
#endif

        rtc6705_set_frequency(5880); // TODO: remove after implementing configuration saving to flash
    }
#endif

    bool last_new_field = new_field;
    while (1)
    {
        bool field_edge_flag = new_field == false && last_new_field == true;
        (void)field_edge_flag;

        last_new_field = new_field;

        msp_loop_process();
        led_blink();
        debug_print_loop();
        logo_timeout_check();
        video_sync_loop();
        msp_menu();

#if defined(USE_VTX) && defined(USE_PA)
        rf_pa_loop(field_edge_flag);
        vtx_power_levels_flush_if_dirty(); // deferred EEPROM write
#endif

#if 0 // TODO: remove later
// For test only - 3D cube animation
#if defined(USE_GRAPHICS)
        if (field_edge_flag) {
            video_draw_3d_cube_animation();
        }
#endif
#endif

    }
}
#endif

void led_blink(void)
{
    static uint32_t last_tick = 0;

    if ((HAL_GetTick() - last_tick) >= LED_BLINK_INTERVAL) {
      led_toggle(LED_STATE);

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
