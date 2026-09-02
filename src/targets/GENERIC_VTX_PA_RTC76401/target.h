/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/GENERIC_VTX_PA_RTC76401/target.h — board variant adding an RTC76401
 * external PA stage downstream of the RTC6705.
 *
 * Differences from targets/GENERIC_VTX/target.h:
 *   - ADC_PA_VDET moves from PB11 to PA4 (RTC76401 pin 19, VPD).
 *     PA4 = ADC2_IN17, confirmed -- NOT reachable from ADC1. adc.c brings
 *     up ADC2 whenever ADC2_NEEDED is derived true (see main.h, right
 *     after it includes this file), which it is here since
 *     ADC_PA_VDET_INSTANCE is ADC_INSTANCE_2.
 *   - PB11 is unconnected on this board -- ADC_RESERVED already covers
 *     "unused analog input" duties, PB11 doesn't need its own entry.
 *   - New PA_ON_Pin/PA_ON_GPIO_Port: RTC76401 VREF enable (PB10), a fast
 *     binary switch per the RTC76401 datasheet -- NOT a PWM/analog control.
 *     rf_pa.c gates its use on #if defined(PA_RTC76401).
 */
#ifndef TARGETS_GENERIC_VTX_PA_RTC76401_TARGET_H
#define TARGETS_GENERIC_VTX_PA_RTC76401_TARGET_H

/* PA type. Downstream code (adc.c, rf_pa.c, ...) gates on this, never on
 * a target/board name. USE_PA (and USE_VTX) come from this board's
 * target.cmake, not from here.
 *   PA_RTC76401 -- RTC76401 external PA, DAC1_OUT2 bias into RTC6705's
 *                  PAOUT1, separate enable GPIO (PA_ON_*), VPD detector
 *                  on its own ADC. */
#define PA_RTC76401

/* rf_pa.c's DAC-bias PID loop gains, operating on a VDET deviation in mV.
 * Unvalidated placeholders -- tune on the bench against this board's
 * actual bias/detector circuit once detector[] targets are populated in
 * targets/GENERIC_VTX_PA_RTC76401/target.c */
#define PA_CONTROL_Kp        0.6f
#define PA_CONTROL_Ki        0.05f
#define PA_CONTROL_Kd        0.0f
#define PA_CONTROL_OFFSET_MV 0u

/* Hard bounds on what the closed loop is ever allowed to command. MIN
 * matches the lowest DAC value bench-confirmed safe WITH the boost stage
 * on (2800mV, ~790mA total, within a 500mA-1A supply's budget) -- the
 * loop can never ask for anything below what's actually been validated,
 * regardless of gains or how wrong a detector[] target turns out to be.
 * MAX is VDD (Q2 off). I_CLAMP is an unvalidated placeholder bounding
 * the integral term's own contribution -- tune alongside the gains. */
#define PA_CONTROL_MV_MIN     2800u
#define PA_CONTROL_MV_MAX     3300u
#define PA_CONTROL_I_CLAMP_MV 300.0f

/* Q2 (P-channel, gate injection into RTC6705's PAOUT1) is INVERTED --
 * lower DAC mV = more conduction = more RF output. Bench-confirmed
 * repeatedly this session (current-draw sweeps, the runaway incident).
 * Do not change without new bench evidence. */
#define PA_DAC_SIGN 1.0f

typedef enum {
  ADC1_CH_RESERVED = 0,
  ADC1_CH_TEMP,
  ADC1_CH_VREF_INT,
  ADC1_CH_COUNT
} adc1_ch_t;

typedef enum {
  ADC2_CH_PA_VDET = 0,
  ADC2_CH_COUNT
} adc2_ch_t;

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
#define ADC_RESERVED_INSTANCE ADC_INSTANCE_1

/* VPD on PA4 -- RTC76401 pin 19. ADC2_IN17 */
#define ADC_PA_VDET_Pin LL_GPIO_PIN_4
#define ADC_PA_VDET_GPIO_Port GPIOA
#define ADC_PA_VDET_Channel LL_ADC_CHANNEL_17
#define ADC_PA_VDET_INSTANCE  ADC_INSTANCE_2

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

#define PA_ON_Pin LL_GPIO_PIN_10
#define PA_ON_GPIO_Port GPIOB

#define USER_KEY_Pin LL_GPIO_PIN_13
#define USER_KEY_GPIO_Port GPIOC

#endif //TARGETS_GENERIC_VTX_PA_RTC76401_TARGET_H
