/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/generic_vtx_pa_rtc76401.h — board variant adding an RTC76401
 * external PA stage downstream of the RTC6705.
 *
 * Differences from targets/generic.h:
 *   - ADC_PA_VDET moves from PB11 to PA4 (RTC76401 pin 19, VPD).
 *     PA4 = ADC2_IN17, confirmed -- NOT reachable from ADC1. adc.c handles
 *     this: on this target, ADC_CH_PA_VDET is read via a dedicated ADC2
 *     single-channel DMA path (see adc2_vdet_init() in adc.c) instead of
 *     ADC1's regular sequence.
 *   - PB11 is unconnected on this board -- ADC_RESERVED already covers
 *     "unused analog input" duties, PB11 doesn't need its own entry.
 *   - New PA_ON_Pin/PA_ON_GPIO_Port: RTC76401 VREF enable (PB10), a fast
 *     binary switch per the RTC76401 datasheet -- NOT a PWM/analog control.
 *     Named PA_ON_* (not RTC76401-specific) to match the existing
 *     #ifdef PA_ON_GPIO_Port convention already present in rf_pa.c.
 */
#ifndef TARGET_GENERIC_VTX_PA_RTC76401_H
#define TARGET_GENERIC_VTX_PA_RTC76401_H

// see adc.c - adc_init()
typedef enum {
  ADC_CH_RESERVED = 0, // reserved
  ADC_CH_PA_VDET = 1, // RTC76401 VPD (pin 19) via PA4
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

/* VPD on PA4 -- RTC76401 pin 19. ADC2_IN17 (confirmed), see adc.c. */
#define ADC_PA_VDET_Pin LL_GPIO_PIN_4
#define ADC_PA_VDET_GPIO_Port GPIOA
#define ADC_PA_VDET_Channel LL_ADC_CHANNEL_17

//
// Reserved pins for future features
//
#define RGBLED_TIM8_CH1_Pin LL_GPIO_PIN_15
#define RGBLED_TIM8_CH1_GPIO_Port GPIOA

#define FRSKY_PIXEL_OSD_TX_USART3_TX_Pin LL_GPIO_PIN_10
#define FRSKY_PIXEL_OSD_TX_USART3_TX_GPIO_Port GPIOC
#define FRSKY_PIXEL_OSD_RX_USART3_RX_Pin LL_GPIO_PIN_11
#define FRSKY_PIXEL_OSD_RX_USART3_RX_GPIO_Port GPIOC

#define FDCAN1_TX_Pin LL_GPIO_PIN_9
#define FDCAN1_TX_GPIO_Port GPIOB
#define FDCAN1_RX_Pin LL_GPIO_PIN_8
#define FDCAN1_RX_GPIO_Port GPIOB

#define RF_VBIAS_DAC1_OUT2_Pin LL_GPIO_PIN_5
#define RF_VBIAS_DAC1_OUT2_GPIO_Port GPIOA

/* RTC76401 VREF enable (PB10) -- fast binary switch, not PWM/analog.
 * See rf_pa.c: guarded with #ifdef PA_ON_GPIO_Port, so builds against
 * targets/generic.h (no external PA stage) still compile untouched. */
#define PA_ON_Pin LL_GPIO_PIN_10
#define PA_ON_GPIO_Port GPIOB

#define USER_KEY_Pin LL_GPIO_PIN_13
#define USER_KEY_GPIO_Port GPIOC

#endif //TARGET_GENERIC_VTX_PA_RTC76401_H
