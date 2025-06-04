/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"

void TIM1_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_TIM1_UP);

    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)&OPAMP1->CSR);

    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_HIGH);

    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_NORMAL);

    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_WORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_WORD);

    // // channel 1 (opamp_buf → OPAMP1->CSR)
    //LL_DMAMUX_SetRequestID(DMAMUX1, LL_DMAMUX_CHANNEL_0, LL_DMAMUX_REQ_TIM1_UP);
    //
    // // channel 2 (dac_buf → DAC3)
    // LL_DMAMUX_SetRequestID(DMAMUX1, LL_DMAMUX_CHANNEL_1, LL_DMAMUX_REQ_DAC3_CH1);

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 24;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM1, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM1);
    LL_TIM_SetClockSource(TIM1, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerInput(TIM1, LL_TIM_TS_ETRF);
    //LL_TIM_SetSlaveMode(TIM1, LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER);
    LL_TIM_SetSlaveMode(TIM1, LL_TIM_SLAVEMODE_RESET);
    LL_TIM_DisableExternalClock(TIM1);
    LL_TIM_ConfigETR(TIM1, LL_TIM_ETR_POLARITY_NONINVERTED, LL_TIM_ETR_PRESCALER_DIV1, LL_TIM_ETR_FILTER_FDIV1);
    LL_TIM_DisableIT_TRIG(TIM1);
    LL_TIM_DisableDMAReq_TRIG(TIM1);
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_UPDATE);
    LL_TIM_DisableMasterSlaveMode(TIM1);
    LL_TIM_SetETRSource(TIM1, LL_TIM_TIM1_ETRSOURCE_COMP3);

    LL_TIM_SetSlaveMode(TIM1, LL_TIM_SLAVEMODE_RESET);
}

void TIM2_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

    /* TIM2 interrupt Init */
    NVIC_SetPriority(TIM2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 4294967295;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(TIM2, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM2);
    LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerInput(TIM2, LL_TIM_TS_ETRF);
    LL_TIM_SetSlaveMode(TIM2, LL_TIM_SLAVEMODE_RESET);
    LL_TIM_DisableExternalClock(TIM2);
    LL_TIM_ConfigETR(TIM2, LL_TIM_ETR_POLARITY_NONINVERTED, LL_TIM_ETR_PRESCALER_DIV1, LL_TIM_ETR_FILTER_FDIV1);
    LL_TIM_DisableIT_TRIG(TIM2);
    LL_TIM_DisableDMAReq_TRIG(TIM2);
    LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM2);
    LL_TIM_SetETRSource(TIM2, LL_TIM_TIM2_ETRSOURCE_COMP3);
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_DIRECTTI);
    LL_TIM_IC_SetPrescaler(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ICPSC_DIV1);
    LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV1);
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_RISING);
    LL_TIM_SetRemap(TIM2, LL_TIM_TIM2_TI1_RMP_COMP3);
}


void TIM3_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

    /* TIM3 interrupt Init */
    NVIC_SetPriority(TIM3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(TIM3_IRQn);

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 65535;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(TIM3, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM3);
    LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerInput(TIM3, LL_TIM_TS_ETRF);
    LL_TIM_SetSlaveMode(TIM3, LL_TIM_SLAVEMODE_RESET);
    LL_TIM_DisableExternalClock(TIM3);
    LL_TIM_ConfigETR(TIM3, LL_TIM_ETR_POLARITY_NONINVERTED, LL_TIM_ETR_PRESCALER_DIV1, LL_TIM_ETR_FILTER_FDIV1);
    LL_TIM_DisableIT_TRIG(TIM3);
    LL_TIM_DisableDMAReq_TRIG(TIM3);
    LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM3);
    LL_TIM_SetETRSource(TIM3, LL_TIM_TIM3_ETRSOURCE_COMP4);
    LL_TIM_IC_SetActiveInput(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_DIRECTTI);
    LL_TIM_IC_SetPrescaler(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_ICPSC_DIV1);
    LL_TIM_IC_SetFilter(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV1);
    LL_TIM_IC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_RISING);
    LL_TIM_SetRemap(TIM3, LL_TIM_TIM3_TI1_RMP_COMP4);
}

void TIM4_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);

    /* TIM4 interrupt Init */
    NVIC_SetPriority(TIM4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));
    NVIC_EnableIRQ(TIM4_IRQn);

    TIM_InitStruct.Prescaler = 16999;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 9999;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(TIM4, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM4);
    LL_TIM_SetClockSource(TIM4, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIM4, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM4);
}


void TIM7_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM7);

    /* TIM7 interrupt Init */
#if defined(STM32G431xx)
    NVIC_SetPriority(TIM7_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));
    NVIC_EnableIRQ(TIM7_IRQn);
#elif defined(STM32G474xx)
    NVIC_SetPriority(TIM7_DAC_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));
    NVIC_EnableIRQ(TIM7_DAC_IRQn);
#endif


    TIM_InitStruct.Prescaler = 169;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 9999; // 10ms
    LL_TIM_Init(TIM7, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM7);
    LL_TIM_SetTriggerOutput(TIM7, LL_TIM_TRGO_UPDATE);
    LL_TIM_DisableMasterSlaveMode(TIM7);
}

void TIM17_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};
    LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
    LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM17);

    /* TIM17 DMA Init */
    /* TIM17_CH1 Init */
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_6, LL_DMAMUX_REQ_TIM17_CH1);

    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_MEDIUM);

    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MODE_CIRCULAR);

    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);

    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_HALFWORD);

    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_HALFWORD);

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 5404;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM17, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM17);
    LL_TIM_OC_EnablePreload(TIM17, LL_TIM_CHANNEL_CH1);
    TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM2;
    TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.CompareValue = 8;
    TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
    TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
    LL_TIM_OC_Init(TIM17, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
    LL_TIM_OC_DisableFast(TIM17, LL_TIM_CHANNEL_CH1);
    TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
    TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
    TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
    TIM_BDTRInitStruct.DeadTime = 0;
    TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE;
    TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
    TIM_BDTRInitStruct.BreakFilter = LL_TIM_BREAK_FILTER_FDIV1;
    TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE;
    LL_TIM_BDTR_Init(TIM17, &TIM_BDTRInitStruct);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    /**TIM17 GPIO Configuration
    PB5     ------> TIM17_CH1 */
    GPIO_InitStruct.Pin = TIM17_CH1_VIDEO_GEN_OUT_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_10;
    LL_GPIO_Init(TIM17_CH1_VIDEO_GEN_OUT_GPIO_Port, &GPIO_InitStruct);
}
