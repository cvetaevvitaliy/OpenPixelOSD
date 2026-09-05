/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"

/*DMA allocation

DMA1 CHANNEL_1    OSD generator OPAMP1 control/status register
DMA1 CHANNEL_2
DMA1 CHANNEL_3    ADC2 g_adc2_dma_buf
DMA1 CHANNEL_4    ADC1 g_adc1_dma_buf
DMA1 CHANNEL_5    Video generator DAC3 data holding register
DMA1 CHANNEL_6    Video generator TIM17 sync pulse timing
DMA1 CHANNEL_7
DMA1 CHANNEL_8    HRTIM1 color carrier generator

DMA2 CHANNEL_1    OSD generator DAC3 data holding register
DMA2 CHANNEL_2    USART1 TX
DMA2 CHANNEL_3    USART1 RX
DMA2 CHANNEL_4    USART3 TX
DMA2 CHANNEL_5    TIM8 RGBLED
DMA2 CHANNEL_6    USART3 RX
DMA2 CHANNEL_7
DMA2 CHANNEL_8
*/

void dma_init(void)
{

  /* Init with LL driver */
  /* DMA controller clock enable */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMAMUX1);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

}
