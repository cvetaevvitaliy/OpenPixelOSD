/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"


void DAC1_Init(void)
{

    LL_DAC_InitTypeDef DAC_InitStruct = {0};

    /* Peripheral clock enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_DAC1);

    /* DAC1 interrupt Init */
    NVIC_SetPriority(TIM6_DAC_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    /** DAC channel OUT1 config */
    LL_DAC_SetHighFrequencyMode(DAC1, LL_DAC_HIGH_FREQ_MODE_ABOVE_160MHZ);
    LL_DAC_SetSignedFormat(DAC1, LL_DAC_CHANNEL_1, LL_DAC_SIGNED_FORMAT_DISABLE);
    DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
    DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
    DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
    DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
    DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
    DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
    LL_DAC_Init(DAC1, LL_DAC_CHANNEL_1, &DAC_InitStruct);
    LL_DAC_DisableTrigger(DAC1, LL_DAC_CHANNEL_1);
    LL_DAC_DisableDMADoubleDataMode(DAC1, LL_DAC_CHANNEL_1);

    /** DAC1 channel OUT2 config */
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    /** DAC1 GPIO Configuration
    PA5   ------> DAC1_OUT2 */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    LL_DAC_SetSignedFormat(DAC1, LL_DAC_CHANNEL_2, LL_DAC_SIGNED_FORMAT_DISABLE);
    DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_ENABLE;
    DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_GPIO;
    LL_DAC_Init(DAC1, LL_DAC_CHANNEL_2, &DAC_InitStruct);
    LL_DAC_DisableTrigger(DAC1, LL_DAC_CHANNEL_2);
    LL_DAC_DisableDMADoubleDataMode(DAC1, LL_DAC_CHANNEL_2);

}

void DAC3_Init(void)
{
    LL_DAC_InitTypeDef DAC_InitStruct = {0};

    /* Peripheral clock enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_DAC3);

    /* DAC3 DMA Init */

    /* DAC3_CH1 Init */
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_TIM1_UP);
    LL_DMA_SetPeriphAddress(DMA2, LL_DMA_CHANNEL_1, (uint32_t)&DAC3->DHR12R1);

    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_VERYHIGH);
    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MODE_NORMAL);

    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_WORD);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_HALFWORD);

    /** DAC channel OUT1 config */
    LL_DAC_SetHighFrequencyMode(DAC3, LL_DAC_HIGH_FREQ_MODE_ABOVE_160MHZ);
    LL_DAC_SetSignedFormat(DAC3, LL_DAC_CHANNEL_1, LL_DAC_SIGNED_FORMAT_DISABLE);
    DAC_InitStruct.TriggerSource = LL_DAC_TRIG_EXT_TIM1_TRGO;
    DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
    DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
    DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
    DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
    DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
    LL_DAC_Init(DAC3, LL_DAC_CHANNEL_1, &DAC_InitStruct);
    LL_DAC_EnableTrigger(DAC3, LL_DAC_CHANNEL_1);

    LL_DAC_DisableDMADoubleDataMode(DAC3, LL_DAC_CHANNEL_1);

    /** DAC channel OUT2 config  */
    LL_DAC_SetSignedFormat(DAC3, LL_DAC_CHANNEL_2, LL_DAC_SIGNED_FORMAT_DISABLE);
    DAC_InitStruct.TriggerSource = LL_DAC_TRIG_EXT_TIM1_TRGO;
    DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
    DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
    DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
    DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
    DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
    LL_DAC_Init(DAC3, LL_DAC_CHANNEL_2, &DAC_InitStruct);
    LL_DAC_DisableTrigger(DAC3, LL_DAC_CHANNEL_2);
    LL_DAC_DisableDMADoubleDataMode(DAC3, LL_DAC_CHANNEL_2);


}
