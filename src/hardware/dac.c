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


}

void DAC3_Init(void)
{
    LL_DAC_InitTypeDef DAC_InitStruct = {0};

    /* Peripheral clock enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_DAC3);

    /* DAC3 DMA Init */

    /* DAC3_CH1 Init */
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_2, LL_DMAMUX_REQ_TIM1_UP); // TIM1_UP is used to trigger DAC3_CH1
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)&DAC3->DHR12R1);

    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_HIGH);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MODE_NORMAL);

    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_WORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_HALFWORD);

    //LL_DMAMUX_SetRequestID(DMAMUX1, LL_DMAMUX_CHANNEL_1, LL_DMAMUX_REQ_DAC3_CH1);

    /* DAC3 interrupt Init */
    NVIC_SetPriority(TIM6_DAC_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    /** DAC channel OUT1 config */
    LL_DAC_SetHighFrequencyMode(DAC3, LL_DAC_HIGH_FREQ_MODE_ABOVE_160MHZ);
    LL_DAC_SetSignedFormat(DAC3, LL_DAC_CHANNEL_1, LL_DAC_SIGNED_FORMAT_DISABLE);
    DAC_InitStruct.TriggerSource = LL_DAC_TRIG_EXT_TIM1_TRGO;
    DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
    DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
    DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE; // Enable output buffer for DAC3
    DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
    DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
    LL_DAC_Init(DAC3, LL_DAC_CHANNEL_1, &DAC_InitStruct);
    LL_DAC_EnableTrigger(DAC3, LL_DAC_CHANNEL_1);

    LL_DAC_DisableDMADoubleDataMode(DAC3, LL_DAC_CHANNEL_1);

    // DAC3 channel 2 provides the adjustable sync threshold for COMP2.
    LL_DAC_SetSignedFormat(DAC3, LL_DAC_CHANNEL_2, LL_DAC_SIGNED_FORMAT_DISABLE);
    DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
    DAC_InitStruct.TriggerSource2 = LL_DAC_TRIG_SOFTWARE;
    DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
    DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_DISABLE;
    DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_INTERNAL;
    DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
    LL_DAC_Init(DAC3, LL_DAC_CHANNEL_2, &DAC_InitStruct);
    LL_DAC_DisableTrigger(DAC3, LL_DAC_CHANNEL_2);
    LL_DAC_DisableDMADoubleDataMode(DAC3, LL_DAC_CHANNEL_2);
}
