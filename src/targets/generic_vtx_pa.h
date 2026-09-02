/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/generic_vtx_pa.h — generic board plus the baseline PA stage
 * (DAC1_OUT2 bias on PA5, VDET on PB11/ADC1, no separate PA-enable GPIO).
 *
 * This is targets/generic.h's pin layout with the PA_GENERIC feature
 * (and USE_PA) turned on. Compare with targets/generic_vtx_pa_rtc76401.h,
 * which instead turns on PA_RTC76401 (different PA type: separate enable
 * GPIO, VDET on ADC2 via PA4).
 *
 * This is also the reference target for PA thermal monitoring via an
 * NTC (see rf_pa.c's rf_pa_ntc_raw_to_celsius() for the assumed circuit
 * and math)
 */
#ifndef TARGET_GENERIC_VTX_PA_H
#define TARGET_GENERIC_VTX_PA_H

/* Feature flags. Downstream code (adc.c, rf_pa.c, ...) gates on these,
 * never on a target/board name -- see targets/target.h.
 *   USE_PA      -- this board has *some* PA stage needing bias/enable/
 *                  detector control. Set by every concrete PA feature
 *                  (PA_GENERIC here, PA_RTC76401 in the other header).
 *   PA_GENERIC  -- this board's specific PA type: DAC1_OUT2 bias (PA5),
 *                  VDET on PB11/ADC1, no separate PA-enable GPIO. */
#define PA_GENERIC
#define USE_PA

/* rf_pa.c's DAC-bias PID loop gains, operating on a VDET deviation in mV.
 * Unvalidated placeholders -- tune on the bench against this board's
 * actual bias/detector circuit once detector[] targets are populated in
 * targets/generic_vtx_pa_power.c. */
#define PA_CONTROL_Kp        0.6f
#define PA_CONTROL_Ki        0.05f
#define PA_CONTROL_Kd        0.0f
#define PA_CONTROL_OFFSET_MV 0u

/* Hard bounds on what the closed loop is ever allowed to command.
 * UNVALIDATED placeholders -- this board's bias circuit has never had a
 * real bench sweep with the boost stage on (see targets/generic_vtx_pa_power.c),
 * unlike the RTC76401 target's bounds which are anchored to bench data.
 * Do not trust these until you've done that sweep. */
#define PA_CONTROL_MV_MIN     2400u
#define PA_CONTROL_MV_MAX     3300u
#define PA_CONTROL_I_CLAMP_MV 300.0f

/* UNCONFIRMED -- this board's bias circuit has never been bench-tested
 * (see targets/generic_vtx_pa_power.c). -1.0f assumes the more typical
 * linear-bias sense (higher DAC = more bias = more output), the OPPOSITE
 * of RTC76401's Q2 gate-injection scheme. Verify on real hardware before
 * trusting the closed loop on this board -- do not assume this is
 * correct just because it's the more common convention. */
#define PA_DAC_SIGN -1.0f

typedef enum {
  ADC1_CH_NTC = 0,
  ADC1_CH_PA_VDET,
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

// Repurposed from ADC_RESERVED (same physical pin: PB1/ADC1_IN12) -- see
// this file's own top comment. NTC circuit: VDDA --[10k pullup]--
// this pin --[10k NTC]-- GND, small cap from this pin to GND. See
// rf_pa.c's rf_pa_ntc_raw_to_celsius() for the conversion math.
#define ADC_NTC_Pin LL_GPIO_PIN_1
#define ADC_NTC_GPIO_Port GPIOB
#define ADC_NTC_Channel LL_ADC_CHANNEL_12
#define ADC_NTC_INSTANCE     ADC_INSTANCE_1

#define ADC_PA_VDET_Pin LL_GPIO_PIN_11
#define ADC_PA_VDET_GPIO_Port GPIOB
#define ADC_PA_VDET_Channel LL_ADC_CHANNEL_14
#define ADC_PA_VDET_INSTANCE ADC_INSTANCE_1

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

// RF PA VBIAS: DAC1_OUT2 controls the VBIAS voltage.
#define RF_VBIAS_DAC1_OUT2_Pin LL_GPIO_PIN_5
#define RF_VBIAS_DAC1_OUT2_GPIO_Port GPIOA

// USER_KEY only used in GPIO init code, currently only used by developers.
#define USER_KEY_Pin LL_GPIO_PIN_13
#define USER_KEY_GPIO_Port GPIOC

#endif //TARGET_GENERIC_VTX_PA_H
