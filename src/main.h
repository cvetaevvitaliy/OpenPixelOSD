
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
#include "stm32g4xx_ll_spi.h"
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

typedef enum {
  PX_BLACK = 0,
  PX_TRANSPARENT,
  PX_WHITE,
  PX_GRAY
} px_t;

// see adc.c - adc_init()
typedef enum {
  ADC_CH_RESERVED = 0, // reserved
  ADC_CH_PA_VDET = 1, // rf pa vdet signal
  ADC_CH_TEMP = 2, // internal temperature sensor
  ADC_CH_VREF_INT  = 3, // internal VREFINT
  ADC_CH_COUNT
} adc_ch_t;

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
// VTX + PA support
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
#define ADC_PA_VDET_Pin LL_GPIO_PIN_11
#define ADC_PA_VDET_GPIO_Port GPIOB
#define ADC_PA_VDET_Channel LL_ADC_CHANNEL_14

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

// If RF PA VBIAS is expanded, then DAC1_OUT1 can be used to control the VBIAS voltage.
#define RF_VBIAS_DAC1_OUT2_Pin LL_GPIO_PIN_5
#define RF_VBIAS_DAC1_OUT2_GPIO_Port GPIOA

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

#define VIDE_DETECTION_MV       (DAC12BIT_TO_MV(250)) // 250 mV for video detection

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

void COMP3_Init(void);
void COMP4_Init(void);

#endif /* __MAIN_H */
