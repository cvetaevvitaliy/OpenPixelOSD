
#ifndef __MAIN_H
#define __MAIN_H

#include "stm32g4xx_hal.h"
#include "stm32g4xx_ll_comp.h"
#include "stm32g4xx_ll_exti.h"
#include "stm32g4xx_ll_dac.h"
#include "stm32g4xx_ll_dma.h"
#include "stm32g4xx_ll_opamp.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_crs.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_usart.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_adc.h"

#ifndef GIT_TAG
#define GIT_TAG "-.-.-"
#endif /* GIT_TAG */

#ifndef GIT_BRANCH
#define GIT_BRANCH ""
#endif /* GIT_BRANCH */

#ifndef GIT_HASH
#define GIT_HASH ""
#endif /* GIT_HASH */

#define FW_VERSION GIT_TAG
#ifndef MCU_TYPE
#define MCU_TYPE "---------"
#endif /* MCU_TYPE */

#define USE_MSP
#define DEBUG_LED_BLINK
#define PAUSE_ON_FAILED_INIT 1

#define INVERT_UART


typedef enum {
  PX_BLACK = 0,
  PX_TRANSPARENT,
  PX_WHITE,
  PX_GRAY
} px_t;

// see adc.c - adc_init()
typedef enum {
  ADC_CH_RESERVED = 0, // reserved
  ADC_CH_TEMP = 1, // internal temperature sensor
  ADC_CH_VREF_INT = 2, // internal VREFINT
  ADC_CH_COUNT
} adc_ch_t;

#define LED_STATE_Pin LL_GPIO_PIN_6
#define LED_STATE_GPIO_Port GPIOC

//
// Dual-camera video detection/overlay
//
// PA3 and PA7 are shared by the COMP2 and OPAMP1 input multiplexers.
// PA0 and the old COMP3 sync route are no longer used.
#define OPAMP1_VINPIO0_GRAY_COLOR_Pin LL_GPIO_PIN_1
#define OPAMP1_VINPIO0_GRAY_COLOR_GPIO_Port GPIOA
#define OPAMP1_VOUT_VIDEO_OUT_Pin LL_GPIO_PIN_2
#define OPAMP1_VOUT_VIDEO_OUT_GPIO_Port GPIOA
#define OPAMP1_VINPIO1_VIDEO1_IN_Pin LL_GPIO_PIN_3
#define OPAMP1_VINPIO1_VIDEO1_IN_GPIO_Port GPIOA
#define OPAMP1_VINPIO2_VIDEO2_IN_Pin LL_GPIO_PIN_7
#define OPAMP1_VINPIO2_VIDEO2_IN_GPIO_Port GPIOA
#define TIM17_CH1_VIDEO_GEN_OUT_Pin LL_GPIO_PIN_5
#define TIM17_CH1_VIDEO_GEN_OUT_GPIO_Port GPIOB

#define ADC_RESERVED_Pin LL_GPIO_PIN_1
#define ADC_RESERVED_GPIO_Port GPIOB
#define ADC_RESERVED_Channel LL_ADC_CHANNEL_12

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

// USER_KEY only used in GPIO init code, currently only used by developers.
#define USER_KEY_Pin LL_GPIO_PIN_13
#define USER_KEY_GPIO_Port GPIOC
// BOOT_KEY only used in GPIO init code, currently only used by developers.
#define BOOT_KEY_Pin LL_GPIO_PIN_8
#define BOOT_KEY_GPIO_Port GPIOB


#define EXEC_RAM __attribute__((section (".ccmram.text"), optimize("Ofast"))) /* exec functions from CCMRAM */
#define CCMRAM_DATA __attribute__((section (".ccmram.data"))) /* initialized var */
#define CCMRAM_BSS __attribute__((section (".ccmram.bss"))) /* uninitialized var */

#define DAC12BIT_TO_MV(value)      (((uint32_t)(value) * 3300) / 4095)
#define DAC12BIT_FROM_MV(mV)       (((uint32_t)(mV) * 4095) / 3300)

#define DAC8BIT_TO_MV(value)      (((uint32_t)(value) * 3300) / 255)
#define DAC8BIT_FROM_MV(mV)       (((uint32_t)(mV) * 255) / 3300)

#define VIDEO_DETECTION_DAC_VALUE DAC12BIT_FROM_MV(270)

void gpio_init(void);
void adc_init(void);
uint16_t adc_read_raw(adc_ch_t ch);
uint16_t adc_read_mv(adc_ch_t ch);
uint32_t adc_read_vdda_mv(void);
float adc_read_mcu_temp_c(void);

void DAC1_Init(void);
void DAC3_Init(void);

void dma_init(void);

void OPAMP1_Init(void);

void TIM1_Init(void);
void TIM2_Init(void);
void TIM3_Init(void);
void TIM4_Init(void);
void TIM7_Init(void);
void TIM17_Init(void);

void COMP2_Init(void);
void COMP4_Init(void);

#endif /* __MAIN_H */
