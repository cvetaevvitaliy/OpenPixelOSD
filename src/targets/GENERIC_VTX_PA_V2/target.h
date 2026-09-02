/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * targets/GENERIC_VTX_PA_V2/target.h — generic RTC6705 VTX board with a
 * Skyworks SE5004L-class linear external PA, on the "optimal" reference
 * pinout (see Cube/OpenPixelOSD.ioc).
 *
 * Successor pin layout to targets/GENERIC_VTX_PA/target.h. This is the
 * layout new boards should start from: every potentially useful peripheral the
 * MCU design can carry has a pin assigned here, either wired to
 * a feature or reserved with a #define for future use.
 *
 * Differences from targets/GENERIC_VTX_PA/target.h (the V1 baseline-PA board):
 *   - The PA stage gains a real enable line. PB1 (Cube label
 *     PA_VREF_OUT) drives the PA's VREF input -- high enables the
 *     amplifier, low disables it. Wired here as PA_ON_*, which is what
 *     rf_pa.c gates its enable/disable + closed loop on; V1 has no
 *     enable pin (DAC bias only).
 *   - PA_VDET moves from PB11/ADC1_IN14 to PA4/ADC2_IN17. PA4 is only
 *     reachable from ADC2, so adc.c brings ADC2 up -- ADC2_NEEDED is
 *     derived from ADC_PA_VDET_INSTANCE below (see main.h).
 *   - The PA thermal NTC moves from PB1/ADC1_IN12 to PB2/ADC2_IN12 (PB1
 *     is now the enable line). Also an ADC2 channel.
 *   - The FRSKY PixelOSD UART moves from USART3 (PC10/PC11) to USART2
 *     (PB3/PB4); PC10/PC11 are reserved for UART4 and PB10/PB11 for
 *     USART3 instead.
 *
 * VBIAS is unchanged: DAC1_OUT2 on PA5.
 */
#ifndef TARGETS_GENERIC_VTX_PA_V2_TARGET_H
#define TARGETS_GENERIC_VTX_PA_V2_TARGET_H

/* USE_PA / USE_VTX come from this board's target.cmake, not from here.
 * The firmware has no "PA type" macro: shared code keys off USE_PA and
 * the concrete resource defines this header provides (PA_ON_Pin,
 * ADC_PA_VDET_*, ADC_NTC_*, RF_VBIAS_DAC1_OUT2_*).
 *
 * This board is an RTC6705 VTX feeding a Skyworks SE5004L (or
 * pin/behaviour-compatible) linear external PA: DAC1_OUT2 bias, a
 * separate binary enable line (PA_ON_*, the PA's VREF input), and a
 * power detector on its own ADC.
 *
 * The PA's bias input is driven by an op-amp gate driver:
 * DAC1_OUT2 feeds the op-amp's inverting input, the op-amp output drives
 * the bias FET's gate, and the FET drain feeds back to the op-amp's
 * non-inverting input. That closes a local loop which linearises the
 * DAC-mV -> RF-output relationship across almost the whole DAC range,
 * breaking down only at the very bottom: below ~25 mV the output does not
 * move at all, and it is only reliably linear from roughly 100 mV up. */

/* rf_pa.c's DAC-bias PID loop gains, operating on a VDET deviation in mV. */
#define PA_CONTROL_Kp        0.6f
#define PA_CONTROL_Ki        0.05f
#define PA_CONTROL_Kd        0.0f
#define PA_CONTROL_OFFSET_MV 0u

/* Hard bounds on what the closed loop is ever allowed to command. The
 * op-amp gate driver above keeps the FET in a safe operating region
 * across the whole DAC span, so these are the DAC's own physical limits
 * rather than a narrower validated window. I_CLAMP bounds the integral
 * term's own contribution. */
#define PA_CONTROL_MV_MIN     0u
#define PA_CONTROL_MV_MAX     3333u
#define PA_CONTROL_I_CLAMP_MV 300.0f

/* Which DAC direction increases RF output. -1.0f = the normal/linear
 * sense: higher DAC mV -> more gate drive -> more bias -> more RF output
 * (see the op-amp gate-driver note above). */
#define PA_DAC_SIGN -1.0f

typedef enum {
  ADC1_CH_TEMP = 0,
  ADC1_CH_VREF_INT,
  ADC1_CH_COUNT
} adc1_ch_t;

/* NTC and PA_VDET both live on ADC2 here (PB2/IN12 and PA4/IN17 --
 * neither is reachable from ADC1). Order MUST match adc2_init()'s
 * registration order in adc.c: RESERVED, then NTC, then PA_VDET. */
typedef enum {
  ADC2_CH_NTC = 0,
  ADC2_CH_PA_VDET,
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

// Alternate video-gen sense input on COMP4 -- reserved for future use
// (Cube: COMP4_INP_VIDEO_GEN_IN).
#define COMP4_INP_VIDEO_GEN_IN_Pin LL_GPIO_PIN_0
#define COMP4_INP_VIDEO_GEN_IN_GPIO_Port GPIOB

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

// PA thermal NTC on PB2 / ADC2_IN12. Circuit assumed by rf_pa.c's
// rf_pa_ntc_raw_to_celsius(): VDDA --[10k pullup]-- this pin --[10k NTC]
// -- GND, small cap from this pin to GND.
#define ADC_NTC_Pin LL_GPIO_PIN_2
#define ADC_NTC_GPIO_Port GPIOB
#define ADC_NTC_Channel LL_ADC_CHANNEL_12
#define ADC_NTC_INSTANCE ADC_INSTANCE_2

// PA output-power detector (VPD) on PA4 / ADC2_IN17 -- NOT reachable from
// ADC1. Setting ADC_PA_VDET_INSTANCE to ADC2 is what makes main.h derive
// ADC2_NEEDED and adc.c bring ADC2 up.
#define ADC_PA_VDET_Pin LL_GPIO_PIN_4
#define ADC_PA_VDET_GPIO_Port GPIOA
#define ADC_PA_VDET_Channel LL_ADC_CHANNEL_17
#define ADC_PA_VDET_INSTANCE ADC_INSTANCE_2

// RF PA VBIAS: DAC1_OUT2 controls the VBIAS voltage.
#define RF_VBIAS_DAC1_OUT2_Pin LL_GPIO_PIN_5
#define RF_VBIAS_DAC1_OUT2_GPIO_Port GPIOA

// PA enable. PB1 drives the PA's VREF input (Cube label PA_VREF_OUT):
// high = PA enabled, low = disabled. Defining PA_ON_Pin is what makes
// rf_pa.c bring the pin up and drive it on enable/disable. If a derived
// board inverts this line (e.g. through a level-shifting FET), also
// define PA_ON_ACTIVE_LOW here.
#define PA_ON_Pin LL_GPIO_PIN_1
#define PA_ON_GPIO_Port GPIOB

//
// Reserved pins for future features
//

// If RGB LED support is added, then TIM8 has required features for driving by DMA.
#define RGBLED_TIM8_CH1_Pin LL_GPIO_PIN_15
#define RGBLED_TIM8_CH1_GPIO_Port GPIOA

// If FRSKY PixelOSD protocol is added, a second UART can be used -- USART2 on this layout.
#define FRSKY_PIXEL_OSD_TX_USART2_TX_Pin LL_GPIO_PIN_3
#define FRSKY_PIXEL_OSD_TX_USART2_TX_GPIO_Port GPIOB
#define FRSKY_PIXEL_OSD_RX_USART2_RX_Pin LL_GPIO_PIN_4
#define FRSKY_PIXEL_OSD_RX_USART2_RX_GPIO_Port GPIOB

// If FDCAN support is added then these pins are required.
#define FDCAN1_TX_Pin LL_GPIO_PIN_9
#define FDCAN1_TX_GPIO_Port GPIOB
#define FDCAN1_RX_Pin LL_GPIO_PIN_8
#define FDCAN1_RX_GPIO_Port GPIOB

// Spare UARTs, broken out and reserved.
#define USART3_TX_RESERVED_Pin LL_GPIO_PIN_10
#define USART3_TX_RESERVED_GPIO_Port GPIOB
#define USART3_RX_RESERVED_Pin LL_GPIO_PIN_11
#define USART3_RX_RESERVED_GPIO_Port GPIOB
#define UART4_TX_RESERVED_Pin LL_GPIO_PIN_10
#define UART4_TX_RESERVED_GPIO_Port GPIOC
#define UART4_RX_RESERVED_Pin LL_GPIO_PIN_11
#define UART4_RX_RESERVED_GPIO_Port GPIOC

// I2C2 broken out and reserved (e.g. for an external EEPROM or peripheral).
#define I2C2_SCL_RESERVED_Pin LL_GPIO_PIN_4
#define I2C2_SCL_RESERVED_GPIO_Port GPIOC
#define I2C2_SDA_RESERVED_Pin LL_GPIO_PIN_8
#define I2C2_SDA_RESERVED_GPIO_Port GPIOA

// TIM16 CH1 / CH1N broken out and reserved.
#define TIM16_CH1_RESERVED_Pin LL_GPIO_PIN_6
#define TIM16_CH1_RESERVED_GPIO_Port GPIOA
#define TIM16_CH1N_RESERVED_Pin LL_GPIO_PIN_6
#define TIM16_CH1N_RESERVED_GPIO_Port GPIOB

// EXTI-capable lines reserved for a future SPI2 device's interrupt pins
// (Cube: SPI2_INT1/INT2_RESERVED). On no-oscillator builds these are the
// OSC32 pins repurposed as GPIO.
#define SPI2_INT1_RESERVED_Pin LL_GPIO_PIN_14
#define SPI2_INT1_RESERVED_GPIO_Port GPIOC
#define SPI2_INT2_RESERVED_Pin LL_GPIO_PIN_15
#define SPI2_INT2_RESERVED_GPIO_Port GPIOC

// USER_KEY only used in GPIO init code, currently only used by developers.
#define USER_KEY_Pin LL_GPIO_PIN_13
#define USER_KEY_GPIO_Port GPIOC

#endif //TARGETS_GENERIC_VTX_PA_V2_TARGET_H
