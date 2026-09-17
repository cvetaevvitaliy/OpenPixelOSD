
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
#include "stm32g4xx_ll_rtc.h"
#include "trace.h"

#include <stdbool.h>

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

#define ADC_INSTANCE_1 1
#define ADC_INSTANCE_2 2

#include "target.h"

// Derives whether ADC2 needs to exist AT ALL from the actual per-channel
// instance claims the target header makes.
#if (defined(ADC_RESERVED_INSTANCE) && ADC_RESERVED_INSTANCE == ADC_INSTANCE_2) || \
    (defined(ADC_PA_VDET_INSTANCE)  && ADC_PA_VDET_INSTANCE  == ADC_INSTANCE_2) || \
    (defined(ADC_NTC_INSTANCE)      && ADC_NTC_INSTANCE      == ADC_INSTANCE_2)
#define ADC2_NEEDED
#endif

#if defined(ADC_RESERVED_INSTANCE)
#if ADC_RESERVED_INSTANCE == ADC_INSTANCE_1
#define ADC_RESERVED_READ_RAW() adc1_read_raw(ADC1_CH_RESERVED)
#define ADC_RESERVED_READ_MV()  adc1_read_mv(ADC1_CH_RESERVED)
#else
#define ADC_RESERVED_READ_RAW() adc2_read_raw(ADC2_CH_RESERVED)
#define ADC_RESERVED_READ_MV()  adc2_read_mv(ADC2_CH_RESERVED)
#endif
#endif

#if defined(ADC_PA_VDET_INSTANCE)
#if ADC_PA_VDET_INSTANCE == ADC_INSTANCE_1
#define ADC_PA_VDET_READ_RAW() adc1_read_raw(ADC1_CH_PA_VDET)
#define ADC_PA_VDET_READ_MV()  adc1_read_mv(ADC1_CH_PA_VDET)
#else
#define ADC_PA_VDET_READ_RAW() adc2_read_raw(ADC2_CH_PA_VDET)
#define ADC_PA_VDET_READ_MV()  adc2_read_mv(ADC2_CH_PA_VDET)
#endif
#endif

#if defined(ADC_NTC_INSTANCE)
#if ADC_NTC_INSTANCE == ADC_INSTANCE_1
#define ADC_NTC_READ_RAW() adc1_read_raw(ADC1_CH_NTC)
#define ADC_NTC_READ_MV()  adc1_read_mv(ADC1_CH_NTC)
#else
#define ADC_NTC_READ_RAW() adc2_read_raw(ADC2_CH_NTC)
#define ADC_NTC_READ_MV()  adc2_read_mv(ADC2_CH_NTC)
#endif
#endif

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
uint16_t adc1_read_raw(adc1_ch_t ch);
uint16_t adc1_read_mv(adc1_ch_t ch);
#if defined(ADC2_NEEDED)
uint16_t adc2_read_raw(adc2_ch_t ch);
uint16_t adc2_read_mv(adc2_ch_t ch);
#endif
uint32_t adc_read_vdda_mv(void);
float adc_read_mcu_temp_c(void);
#if defined(ADC2_NEEDED)
void adc2_vdet_debug_status(bool *adc_enabled, bool *adc_ready, bool *dma_enabled, uint16_t *dma_remaining);
#endif

void DAC1_Init(void);
void DAC3_Init(void);

void dma_init(void);

void OPAMP1_Init(void);

void TIM1_Init(void);
void TIM2_Init(void);
void TIM3_Init(void);
void TIM4_Init(void);
void TIM8_Init(void);
void TIM7_Init(void);
void TIM17_Init(void);

void COMP3_Init(void);
void COMP4_Init(void);

#endif /* __MAIN_H */
