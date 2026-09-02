/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/generic.h — baseline board pin/resource definitions.
 *
 * No PA feature of any kind (USE_PA is never defined here) -- suitable
 * for a board with an RTC6705 but no PA stage to control. For boards
 * with a PA, see targets/generic_vtx_pa.h (baseline PA) or
 * targets/generic_vtx_pa_rtc76401.h (RTC76401 external PA).
 */
#ifndef TARGET_GENERIC_H
#define TARGET_GENERIC_H

typedef enum {
  ADC1_CH_RESERVED = 0,
  ADC1_CH_TEMP,
  ADC1_CH_VREF_INT,
  ADC1_CH_COUNT
} adc1_ch_t;

#define LED_STATE_Pin LL_GPIO_PIN_6
#define LED_STATE_GPIO_Port GPIOC

//
// Video detection/generation/overlay
//
#define COMP3_INP_VIDEO_IN_Pin LL_GPIO_PIN_0
#define COMP3_INP_VIDEO_IN_GPIO_Port GPIOA
#define OPAMP1_VINPIO0_GRAY_COLOR_Pin LL_GPIO_PIN_1
#define OPAMP1_VINPIO0_GRAY_COLOR_GPIO_Port GPIOA
#define OPAMP1_VOUT_VIDEO_OUT_Pin LL_GPIO_PIN_2
#define OPAMP1_VOUT_VIDEO_OUT_GPIO_Port GPIOA
#define OPAMP1_VINPIO0_VIDEO_GEN_IN_Pin LL_GPIO_PIN_3
#define OPAMP1_VINPIO0_VIDEO_GEN_IN_GPIO_Port GPIOA
#define OPAMP1_VINPIO2_VIDEO_IN_Pin LL_GPIO_PIN_7
#define OPAMP1_VINPIO2_VIDEO_IN_GPIO_Port GPIOA
#define TIM17_CH1_VIDEO_GEN_OUT_Pin LL_GPIO_PIN_5
#define TIM17_CH1_VIDEO_GEN_OUT_GPIO_Port GPIOB
#define COMP3_OUT_SYNC_EXT_TRIGGER_Pin LL_GPIO_PIN_7
#define COMP3_OUT_SYNC_EXT_TRIGGER_GPIO_Port GPIOB

//
// VTX support (RTC6705, no PA)
//

// RTC6705 is driven by software, but using the same pins that would be used if it was driven in hardware.
// If an SPI based RTC6705 replacement is available in the future, fewer changes would have to be made in both hardware
// designs and software to accomodate this.
//
// For an RTC6705, when using hardware SPI MISO and MOSI can be connected to each other via a 330R resistor,
// and then MISO is connected to the RTC6705's SPIDATA signal, in this configuration either hardware or software
// can be used, clocking out 32 bits instead of the usual 25.
//
// Currently the code uses bitbanged IO to the RTC6705, using SPI2_MOSI/CLK/CS, see rtc6705.c defines.
#define SPI2_CS_Pin LL_GPIO_PIN_12
#define SPI2_CS_GPIO_Port GPIOB
#define SPI2_SCK_Pin LL_GPIO_PIN_13
#define SPI2_SCK_GPIO_Port GPIOB
#define SPI2_MISO_Pin LL_GPIO_PIN_14
#define SPI2_MISO_GPIO_Port GPIOB
#define SPI2_MOSI_Pin LL_GPIO_PIN_15
#define SPI2_MOSI_GPIO_Port GPIOB

#define ADC_RESERVED_Pin LL_GPIO_PIN_1
#define ADC_RESERVED_GPIO_Port GPIOB
#define ADC_RESERVED_Channel LL_ADC_CHANNEL_12
#define ADC_RESERVED_INSTANCE ADC_INSTANCE_1

//
// Reserved pins for future features
//

// If RGB LED support is added, then TIM8 has required features for driving by DMA.
#define RGBLED_TIM8_CH1_Pin LL_GPIO_PIN_15
#define RGBLED_TIM8_CH1_GPIO_Port GPIOA

// If FRSKY PixelOSD protocol is added, a second UART can be used.
#define FRSKY_PIXEL_OSD_TX_USART3_TX_Pin LL_GPIO_PIN_10
#define FRSKY_PIXEL_OSD_TX_USART3_TX_GPIO_Port GPIOC
#define FRSKY_PIXEL_OSD_RX_USART3_RX_Pin LL_GPIO_PIN_11
#define FRSKY_PIXEL_OSD_RX_USART3_RX_GPIO_Port GPIOC

// If FDCAN support is added then these pins are required.
#define FDCAN1_TX_Pin LL_GPIO_PIN_9
#define FDCAN1_TX_GPIO_Port GPIOB
#define FDCAN1_RX_Pin LL_GPIO_PIN_8
#define FDCAN1_RX_GPIO_Port GPIOB

// USER_KEY only used in GPIO init code, currently only used by developers.
#define USER_KEY_Pin LL_GPIO_PIN_13
#define USER_KEY_GPIO_Port GPIOC

#endif //TARGET_GENERIC_H
