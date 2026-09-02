/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/GENERIC/target.h — OSD-only board pin/resource definitions.
 *
 * No VTX of any kind: no RTC6705 (no SPI2 pins), no PA (USE_PA never
 * defined). Suitable for a plain video OSD board. For a board with an
 * RTC6705 VTX, see targets/GENERIC_VTX/target.h; for boards that also
 * have a PA stage, see targets/GENERIC_VTX_PA/target.h or
 * targets/GENERIC_VTX_PA_RTC76401/target.h.
 */
#ifndef TARGETS_GENERIC_TARGET_H
#define TARGETS_GENERIC_TARGET_H

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

#endif //TARGETS_GENERIC_TARGET_H
