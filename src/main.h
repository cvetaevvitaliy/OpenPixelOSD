
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
#include "stm32g4xx_ll_hrtim.h"
#include "stm32g4xx_ll_usart.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_adc.h"
#include "stm32g4xx_ll_rtc.h"
#include "trace.h"
#include "target.h"

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

#if defined(STM32G474xx) && defined(USE_COLOR)
#define IF_USE_COLOR(arg)                       arg
#else
#undef  USE_COLOR
#define IF_USE_COLOR(...)                       { }
#endif

#if !defined(STM32G474xx) 
#undef  USE_GRAPHICS
#endif

#if defined(USE_GRAPHICS) && defined(USE_COLOR)
#undef USE_HD
#define ROW_SIZE                                15
#define COLUMN_SIZE                             32
#define OPAMP_DELAY                             13
#define COLOR_DELAY_PAL                         70
#define COLOR_DELAY_NTSC                        88
#elif defined(USE_GRAPHICS) && defined(USE_HD)
#undef USE_HD
#define ROW_SIZE                                15
#define COLUMN_SIZE                             45
#define OPAMP_DELAY                             10
#elif defined(USE_GRAPHICS)
#define ROW_SIZE                                15
#define COLUMN_SIZE                             32
#define OPAMP_DELAY                             13
#elif defined(USE_HD) && defined(USE_COLOR)
#define ROW_SIZE                                30
#define COLUMN_SIZE                             40
#define OPAMP_DELAY                             10
#define COLOR_DELAY_PAL                         70
#define COLOR_DELAY_NTSC                        88
#elif defined(USE_HD)
#define ROW_SIZE                                30
#define COLUMN_SIZE                             45
#define OPAMP_DELAY                             6
#elif defined(USE_COLOR)
#define ROW_SIZE                                15
#define COLUMN_SIZE                             36
#define OPAMP_DELAY                             12
#define COLOR_DELAY_PAL                         76
#define COLOR_DELAY_NTSC                        88
#else
#define ROW_SIZE                                15
#define COLUMN_SIZE                             36
#define OPAMP_DELAY                             16
#endif


// Video input 1 PA7
#ifndef VIDEO1_INPUT_ENABLED 
#define VIDEO1_INPUT_ENABLED                    true
#endif
#ifndef VIDEO1_INPUT_GAIN
#define VIDEO1_INPUT_GAIN                       1
#endif

// Video input 2 PA3
#ifndef VIDEO2_INPUT_ENABLED 
#define VIDEO2_INPUT_ENABLED                    false
#endif
#ifndef VIDEO2_INPUT_GAIN
#define VIDEO2_INPUT_GAIN                       1
#endif

#ifndef VIDEO_TOTAL_GAIN
#define VIDEO_TOTAL_GAIN                        1
#endif

#define VISUAL_PICTURE_LINE_NS                  50000
#define LINE_CENTER_NS                          31400

#define NS_TO_TICKS(ns)                         (((ns) * 170UL) / 1000UL)
#define VISUAL_PICTURE_LINE_TICKS_MAX           (NS_TO_TICKS(VISUAL_PICTURE_LINE_NS))
#define PIXELS_PER_LINE                         (COLUMN_SIZE * 12)
#define TIM1_AUTORELOAD                         ((uint32_t)(VISUAL_PICTURE_LINE_TICKS_MAX / PIXELS_PER_LINE) - 1)
#define VISUAL_PICTURE_LINE_TICKS               ((TIM1_AUTORELOAD + 1) * PIXELS_PER_LINE)
#define LINE_START_DELAY                        (NS_TO_TICKS(LINE_CENTER_NS) - (VISUAL_PICTURE_LINE_TICKS) / 2)

#define BLACK_LEVEL_ADC_DELAY_NS                3300
#define LOW_SYNC_ADC_DELAY_NS                   6000
#define COLOR_BURST_SYNC_GATE_CLOSE_NS          2100
#define VISIBLE_LINE_END_NS                     57000

typedef enum {
  PX_BLACK = 0,
  PX_TRANSPARENT,
  PX_WHITE,
  PX_GRAY,
  PX_GREEN,
  PX_RED,
  PX_BLUE,
  PX_YELLOW
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


#define OPAMP1_VOUT_VIDEO_OUT_Pin               LL_GPIO_PIN_2
#define OPAMP1_VOUT_VIDEO_OUT_GPIO_Port         GPIOA

#define OPAMP1_VINPIO0_VIDEO1_IN_Pin            LL_GPIO_PIN_7
#define OPAMP1_VINPIO0_VIDEO1_IN_GPIO_Port      GPIOA

#define OPAMP1_VINPIO2_VIDEO2_IN_Pin            LL_GPIO_PIN_3
#define OPAMP1_VINPIO2_VIDEO2_IN_GPIO_Port      GPIOA


#define EXEC_RAM      __attribute__((section (".ccmram.text"), optimize("Ofast"))) /* exec functions from CCMRAM */
#define CCMRAM_DATA   __attribute__((section (".ccmram.data"))) /* initialized var */
#define CCMRAM_BSS    __attribute__((section (".ccmram.bss"))) /* uninitialized var */

#define DAC12BIT_TO_MV(value)                   (((uint32_t)(value) * 3300) / 4095)
#define DAC12BIT_FROM_MV(mV)                    (((uint32_t)(mV) * 4095) / 3300)

#define DAC8BIT_TO_MV(value)                    (((uint32_t)(value) * 3300) / 255)
#define DAC8BIT_FROM_MV(mV)                     (((uint32_t)(mV) * 255) / 3300)

#define SYNC_START_MV                           300
#define SYNC_SCAN_MIN_MV                        25
#define SYNC_SCAN_MAX_MV                        800
#define SYNC_SCAN_INC_MV                        25

#define SYNC_LOST_FRAMES_THRESHOLD              20


#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

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
void TIM7_Init(void);
void TIM8_Init(void);
void TIM15_Init(void);
void TIM17_Init(void);
void HRTIM1_Init(void);

void COMP2_Init(void);
void COMP3_Init(void);

#endif /* __MAIN_H */
