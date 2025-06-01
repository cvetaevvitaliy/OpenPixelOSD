
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

#define USER_KEY_Pin LL_GPIO_PIN_13
#define USER_KEY_GPIO_Port GPIOC
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
#define SPI2_CS_Pin LL_GPIO_PIN_14
#define SPI2_CS_GPIO_Port GPIOB
#define LED_STATE_Pin LL_GPIO_PIN_6
#define LED_STATE_GPIO_Port GPIOC
#define TIM17_CH1_VIDEO_GEN_OUT_Pin LL_GPIO_PIN_5
#define TIM17_CH1_VIDEO_GEN_OUT_GPIO_Port GPIOB
#define COMP3_OUT_SYNC_EXT_TRIGGER_Pin LL_GPIO_PIN_7
#define COMP3_OUT_SYNC_EXT_TRIGGER_GPIO_Port GPIOB
#define BOOT_KEY_Pin LL_GPIO_PIN_8
#define BOOT_KEY_GPIO_Port GPIOB

#define EXEC_RAM __attribute__((section (".ccmram.text"), optimize("Ofast"))) /* exec functions from CCMRAM */
#define CCMRAM_DATA __attribute__((section (".ccmram.data"))) /* initialized var */
#define CCMRAM_BSS __attribute__((section (".ccmram.bss"))) /* uninitialized var */

#define DAC12BIT_TO_MV(value)      (((uint32_t)(value) * 3300) / 4095)
#define DAC12BIT_FROM_MV(mV)       (((uint32_t)(mV) * 4095) / 3300)

#define DAC8BIT_TO_MV(value)      (((uint32_t)(value) * 3300) / 255)
#define DAC8BIT_FROM_MV(mV)       (((uint32_t)(mV) * 255) / 3300)


void gpio_init(void);

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
