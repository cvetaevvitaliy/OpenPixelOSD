#include "main.h"
#include "led.h"

#define RGB_TOGGLE_BIT          (1<<24)

extern uint32_t opamp_buff[];

uint32_t rgbLed[RGB_LED_COUNT];
uint8_t* rgb_buffer;

void led_init(void)
{
    TIM8_Init();
}   

void led_set(uint8_t idx, uint32_t value) {
  GPIO_TypeDef *GPIOx;
  uint32_t PinMask;

  if(idx < RGB_LED_COUNT) {

    rgbLed[idx] = value;
    return;
  } else if (idx == LED_STATE) {
    GPIOx = LED_STATE_GPIO_Port;
    PinMask = LED_STATE_Pin;
  #ifdef TP1_Pin
  } else if (idx == TP1) {
    GPIOx = TP1_GPIO_Port;
    PinMask = TP1_Pin;
  #endif
  #ifdef TP2_Pin
  } else if (idx == TP2) {
    GPIOx = TP2_GPIO_Port;
    PinMask = TP2_Pin;
  #endif
  } else {
    return;
  }

  if (value) {
    LL_GPIO_SetOutputPin(GPIOx, PinMask);
  } else {
    LL_GPIO_ResetOutputPin(GPIOx, PinMask);
  }

}

void led_toggle(uint8_t idx) {
  GPIO_TypeDef *GPIOx;
  uint32_t PinMask;

  if(idx < RGB_LED_COUNT) {
    rgbLed[idx] ^= RGB_TOGGLE_BIT;
    return;
  } else if (idx == LED_STATE) {
    GPIOx = LED_STATE_GPIO_Port;
    PinMask = LED_STATE_Pin;
  #ifdef TP1_Pin
  } else if (idx == TP1) {
    GPIOx = TP1_GPIO_Port;
    PinMask = TP1_Pin;
  #endif
  #ifdef TP2_Pin
  } else if (idx == TP2) {
    GPIOx = TP2_GPIO_Port;
    PinMask = TP2_Pin;
  #endif
  } else {
    return;
  }

  LL_GPIO_TogglePin(GPIOx, PinMask);
}

void RGB_led_send(void) {
  rgb_buffer = (uint8_t*)opamp_buff;

  uint8_t idx = 0;
  rgb_buffer[idx++] = 0;

  for(uint8_t c=0; c<RGB_LED_COUNT; c++) {
    
      for(uint8_t x=0; x<24; x++) {
        if(!(rgbLed[c] & RGB_TOGGLE_BIT) && ((rgbLed[c]<<x) & 0x800000)) {
          rgb_buffer[idx] = 148;
        } else {
          rgb_buffer[idx] = 64;
        }
        idx++;
      }
  }
  rgb_buffer[idx++] = 0;

  LL_TIM_DisableCounter(TIM8);
  LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_5);
  LL_DMA_SetMemoryAddress(DMA2, LL_DMA_CHANNEL_5, (uint32_t)&rgb_buffer[0]);
  LL_DMA_SetPeriphAddress(DMA2, LL_DMA_CHANNEL_5, (uint32_t)&TIM8->CCR1);
  LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_5, idx);
  
  LL_TIM_OC_SetCompareCH1(TIM8, 0);
  LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_5);
  LL_TIM_EnableCounter(TIM8);

}
