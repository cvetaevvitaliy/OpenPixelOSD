/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"
#include "video_overlay.h"

void TIM1_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_TIM1_CH2);

    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)&OPAMP1->CSR);

    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_VERYHIGH);

    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MODE_NORMAL);

    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_WORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_WORD);

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = TIM1_AUTORELOAD;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM1, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM1);
    LL_TIM_SetClockSource(TIM1, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetSlaveMode(TIM1, LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER);
    LL_TIM_DisableExternalClock(TIM1);
    LL_TIM_SetTriggerInput(TIM1, LL_TIM_TS_ITR6);
    LL_TIM_DisableIT_TRIG(TIM1);
    LL_TIM_DisableDMAReq_TRIG(TIM1);
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_UPDATE);
    LL_TIM_DisableMasterSlaveMode(TIM1);

    LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
    TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
    TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.CompareValue = TIM1_AUTORELOAD/2;
    TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
    TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
    LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);

    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);

    TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
    TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.CompareValue = OPAMP_DELAY;
    TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
    TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
    LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
    
    LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
    
    LL_TIM_EnableCounter(TIM1);
}

void TIM2_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

    /* TIM2 interrupt Init */
    NVIC_SetPriority(TIM2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),1, 0));
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
    LL_TIM_ConfigETR(TIM2, LL_TIM_ETR_POLARITY_NONINVERTED, LL_TIM_ETR_PRESCALER_DIV1, LL_TIM_ETR_FILTER_FDIV4_N8);
    LL_TIM_DisableIT_TRIG(TIM2);
    LL_TIM_DisableDMAReq_TRIG(TIM2);
    LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM2);
    
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_ACTIVEINPUT_DIRECTTI);
    LL_TIM_IC_SetPrescaler(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_ICPSC_DIV1);
    LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_FILTER_FDIV4_N6);
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_POLARITY_RISING);

    LL_TIM_SetETRSource(TIM2, LL_TIM_TIM2_ETRSOURCE_COMP2);
    LL_TIM_SetRemap(TIM2, LL_TIM_TIM2_TI2_RMP_COMP2);

    LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
    TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
    TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.CompareValue = NS_TO_TICKS(BLACK_LEVEL_ADC_DELAY_NS);
    TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
    LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
    LL_TIM_OC_DisableFast(TIM2, LL_TIM_CHANNEL_CH1);

    TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
    TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.CompareValue = NS_TO_TICKS(VISIBLE_LINE_END_NS);
    TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
    LL_TIM_OC_Init(TIM2, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);
    LL_TIM_OC_DisableFast(TIM2, LL_TIM_CHANNEL_CH3);

    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH3);
    LL_TIM_EnableAllOutputs(TIM2);
}

#ifdef USE_COLOR
void TIM3_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

  NVIC_SetPriority(TIM3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
  NVIC_EnableIRQ(TIM3_IRQn);

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

  TIM_InitStruct.Prescaler = 0;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 65535;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM3, &TIM_InitStruct);

  LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_SetTriggerInput(TIM3, LL_TIM_TS_ETRF);
  LL_TIM_SetSlaveMode(TIM3, LL_TIM_SLAVEMODE_RESET);
  LL_TIM_DisableExternalClock(TIM3);
  LL_TIM_ConfigETR(TIM3, LL_TIM_ETR_POLARITY_NONINVERTED, LL_TIM_ETR_PRESCALER_DIV1, LL_TIM_ETR_FILTER_FDIV4_N8);
  LL_TIM_DisableIT_TRIG(TIM3);
  LL_TIM_DisableDMAReq_TRIG(TIM3);
  LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_CC1IF);
  LL_TIM_DisableMasterSlaveMode(TIM3);

  LL_TIM_SetETRSource(TIM3, LL_TIM_TIM3_ETRSOURCE_COMP2);

  LL_TIM_DisableARRPreload(TIM3);
  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.CompareValue = NS_TO_TICKS(COLOR_BURST_SYNC_GATE_CLOSE_NS);
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_LOW;
  LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
  LL_TIM_OC_DisableFast(TIM3, LL_TIM_CHANNEL_CH1);

  #if 0
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);
  LL_TIM_EnableAllOutputs(TIM3);
  #endif
  
}
#endif

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

void TIM8_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* RGB LED DMA */
  LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_5, LL_DMAMUX_REQ_TIM8_UP);
  LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_5, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
  LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_5, LL_DMA_PRIORITY_MEDIUM);
  LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_5, LL_DMA_MODE_NORMAL);
  LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_5, LL_DMA_PERIPH_NOINCREMENT);
  LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_5, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_5, LL_DMA_PDATAALIGN_HALFWORD);
  LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_5, LL_DMA_MDATAALIGN_BYTE);


  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM8);

  TIM_InitStruct.Prescaler = 0;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 212;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  TIM_InitStruct.RepetitionCounter = 0;
  LL_TIM_Init(TIM8, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM8);
  LL_TIM_SetClockSource(TIM8, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_OC_EnablePreload(TIM8, LL_TIM_CHANNEL_CH1);
  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
  TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.CompareValue = 0;
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
  TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
  TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
  LL_TIM_OC_Init(TIM8, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
  LL_TIM_OC_DisableFast(TIM8, LL_TIM_CHANNEL_CH1);
  LL_TIM_SetTriggerOutput(TIM8, LL_TIM_TRGO_UPDATE);
  LL_TIM_SetTriggerOutput2(TIM8, LL_TIM_TRGO2_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM8);

  LL_TIM_EnableCounter(TIM8);
  LL_TIM_CC_EnableChannel(TIM8, LL_TIM_CHANNEL_CH1);
  LL_TIM_EnableAllOutputs(TIM8);

  LL_TIM_EnableDMAReq_UPDATE(TIM8);

#if defined(RGBLED_TIM8_CH1_Pin)
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    /**TIM8 GPIO Configuration
    PA15     ------> TIM8_CH1
    */
  GPIO_InitStruct.Pin = RGBLED_TIM8_CH1_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(RGBLED_TIM8_CH1_GPIO_Port, &GPIO_InitStruct);
#endif

}

void TIM15_Init(void)
{

    LL_TIM_InitTypeDef TIM_InitStruct = {0};
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM15);

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = LINE_START_DELAY;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM15, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM15);
    LL_TIM_SetClockSource(TIM15, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetOnePulseMode(TIM15, LL_TIM_ONEPULSEMODE_SINGLE);
    LL_TIM_SetTriggerInput(TIM15, LL_TIM_TS_ITR1);
    LL_TIM_SetSlaveMode(TIM15, LL_TIM_SLAVEMODE_COMBINED_RESETTRIGGER);
    LL_TIM_DisableIT_TRIG(TIM15);
    LL_TIM_DisableDMAReq_TRIG(TIM15);
    LL_TIM_SetTriggerOutput(TIM15, LL_TIM_TRGO_UPDATE);
    LL_TIM_DisableMasterSlaveMode(TIM15);
    
    LL_TIM_EnableDMAReq_UPDATE(TIM15);

}

void TIM17_Init(void)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};
    LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM17);

    /* TIM17 interrupt Init */
    NVIC_SetPriority(TIM1_TRG_COM_TIM17_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),1, 0));
    NVIC_EnableIRQ(TIM1_TRG_COM_TIM17_IRQn);

    /* TIM17 DMA Init */
    /* Video timing DMA */
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_6, LL_DMAMUX_REQ_TIM17_UP);
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_MEDIUM);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_HALFWORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_HALFWORD);

    /* Sync generator DMA*/
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_5, LL_DMAMUX_REQ_TIM17_UP);
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_5, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_5, LL_DMA_PRIORITY_MEDIUM);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_5, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_5, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_5, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_5, LL_DMA_PDATAALIGN_WORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_5, LL_DMA_MDATAALIGN_HALFWORD);
    
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&video_levels);     
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&DAC3->DHR12R1);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, 2);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 5404;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM17, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM17);
    LL_TIM_OC_EnablePreload(TIM17, LL_TIM_CHANNEL_CH1);
    TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
    TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.CompareValue = 20;
    TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
    TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
    LL_TIM_OC_Init(TIM17, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
    LL_TIM_OC_DisableFast(TIM17, LL_TIM_CHANNEL_CH1);

    LL_TIM_EnableDMAReq_CC1(TIM17);
    LL_TIM_EnableDMAReq_UPDATE(TIM17);

}

#ifdef USE_COLOR
void HRTIM1_Init(void)
{ 

  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_HRTIM1);

  // Color phase shift DMA 
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_8, LL_DMAMUX_REQ_TIM1_CH1);
  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_8, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
  LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_8, (uint32_t)&HRTIM1->sTimerxRegs[1].CMP1CxR);
  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_8, LL_DMA_PRIORITY_HIGH);
  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_8, LL_DMA_MODE_NORMAL);
  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_8, LL_DMA_PERIPH_NOINCREMENT);
  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_8, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_8, LL_DMA_PDATAALIGN_WORD);
  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_8, LL_DMA_MDATAALIGN_WORD);  


  LL_HRTIM_SetSyncOutSrc(HRTIM1, LL_HRTIM_SYNCOUT_SRC_TIMA_CMP1);
  LL_HRTIM_SetSyncOutConfig(HRTIM1, LL_HRTIM_SYNCOUT_POSITIVE_PULSE);
  LL_HRTIM_SetSyncInSrc(HRTIM1, LL_HRTIM_SYNCIN_SRC_EXTERNAL_EVENT);
  LL_HRTIM_ConfigDLLCalibration(HRTIM1, LL_HRTIM_DLLCALIBRATION_MODE_SINGLESHOT, LL_HRTIM_DLLCALIBRATION_RATE_0);
  LL_HRTIM_StartDLLCalibration(HRTIM1);

  while(LL_HRTIM_IsActiveFlag_DLLRDY(HRTIM1) == RESET){}

  // External event COMP2
  LL_HRTIM_EE_SetPrescaler(HRTIM1, LL_HRTIM_EE_PRESCALER_DIV1);
  LL_HRTIM_EE_SetSrc(HRTIM1, LL_HRTIM_EVENT_1, LL_HRTIM_EEV1SRC_COMP2_OUT);
  LL_HRTIM_EE_SetPolarity(HRTIM1, LL_HRTIM_EVENT_1, LL_HRTIM_EE_POLARITY_HIGH);
  LL_HRTIM_EE_SetSensitivity(HRTIM1, LL_HRTIM_EVENT_1, LL_HRTIM_EE_SENSITIVITY_LEVEL);
  LL_HRTIM_EE_SetFastMode(HRTIM1, LL_HRTIM_EVENT_1, LL_HRTIM_EE_FASTMODE_ENABLE);

  while(LL_HRTIM_IsActiveFlag_DLLRDY(HRTIM1) == RESET){}

  // TIMER A (phase output)
  LL_HRTIM_TIM_SetPrescaler(HRTIM1, LL_HRTIM_TIMER_A, LL_HRTIM_PRESCALERRATIO_MUL32);
  LL_HRTIM_TIM_SetCounterMode(HRTIM1, LL_HRTIM_TIMER_A, LL_HRTIM_MODE_CONTINUOUS);
  LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_A, 1227);

  LL_HRTIM_TIM_EnableStartOnSync(HRTIM1, LL_HRTIM_TIMER_A);
  LL_HRTIM_TIM_EnableResetOnSync(HRTIM1, LL_HRTIM_TIMER_A);
  LL_HRTIM_TIM_DisableStartOnSync(HRTIM1, LL_HRTIM_TIMER_A);
  LL_HRTIM_TIM_DisableResetOnSync(HRTIM1, LL_HRTIM_TIMER_A);
  LL_HRTIM_TIM_SetUpdateTrig(HRTIM1, LL_HRTIM_TIMER_A, LL_HRTIM_UPDATETRIG_RESET);
  LL_HRTIM_TIM_SetResetTrig(HRTIM1, LL_HRTIM_TIMER_A, LL_HRTIM_RESETTRIG_OTHER1_CMP1);
 
  LL_HRTIM_TIM_DisablePreload(HRTIM1, LL_HRTIM_TIMER_A);
  LL_HRTIM_TIM_EnableResyncUpdate(HRTIM1, LL_HRTIM_TIMER_A);
  LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_A, 0);
  LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_A, 614);

  LL_HRTIM_ForceUpdate(HRTIM1, LL_HRTIM_TIMER_A);
  LL_HRTIM_TIM_CounterEnable(HRTIM1, LL_HRTIM_TIMER_A);

  // TIMER B (phase sync timer)
  LL_HRTIM_TIM_SetPrescaler(HRTIM1, LL_HRTIM_TIMER_B, LL_HRTIM_PRESCALERRATIO_MUL32);
  LL_HRTIM_TIM_SetCounterMode(HRTIM1, LL_HRTIM_TIMER_B, LL_HRTIM_MODE_CONTINUOUS);
  LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_B, 1227);

  LL_HRTIM_TIM_EnableStartOnSync(HRTIM1, LL_HRTIM_TIMER_B);
  LL_HRTIM_TIM_EnableResetOnSync(HRTIM1, LL_HRTIM_TIMER_B);
  LL_HRTIM_TIM_SetUpdateTrig(HRTIM1, LL_HRTIM_TIMER_B, LL_HRTIM_UPDATETRIG_NONE);
  LL_HRTIM_TIM_SetResetTrig(HRTIM1, LL_HRTIM_TIMER_B, LL_HRTIM_RESETTRIG_EEV_1);

  LL_HRTIM_TIM_DisablePreload(HRTIM1, LL_HRTIM_TIMER_B);
  LL_HRTIM_TIM_EnableResyncUpdate(HRTIM1, LL_HRTIM_TIMER_B);
  LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_B, 614);
  LL_HRTIM_TIM_SetCompare2(HRTIM1, LL_HRTIM_TIMER_B, 614);
  LL_HRTIM_TIM_SetCaptureTrig(HRTIM1, LL_HRTIM_TIMER_B, LL_HRTIM_CAPTUREUNIT_1, LL_HRTIM_CAPTURETRIG_TIMC_CMP1);

  LL_HRTIM_ForceUpdate(HRTIM1, LL_HRTIM_TIMER_B);
  LL_HRTIM_TIM_CounterEnable(HRTIM1, LL_HRTIM_TIMER_B);

  while(LL_HRTIM_IsActiveFlag_DLLRDY(HRTIM1) == RESET){}

  // TIMER C (phase shift reference timer)
  LL_HRTIM_TIM_SetPrescaler(HRTIM1, LL_HRTIM_TIMER_C, LL_HRTIM_PRESCALERRATIO_MUL32);
  LL_HRTIM_TIM_SetCounterMode(HRTIM1, LL_HRTIM_TIMER_C, LL_HRTIM_MODE_CONTINUOUS);
  LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_C, 1227);

  LL_HRTIM_TIM_DisableStartOnSync(HRTIM1, LL_HRTIM_TIMER_C);
  LL_HRTIM_TIM_EnableResetOnSync(HRTIM1, LL_HRTIM_TIMER_C);
  LL_HRTIM_TIM_SetUpdateTrig(HRTIM1, LL_HRTIM_TIMER_C, LL_HRTIM_UPDATETRIG_NONE);
  LL_HRTIM_TIM_SetResetTrig(HRTIM1, LL_HRTIM_TIMER_C, LL_HRTIM_RESETTRIG_OTHER2_CMP2);

  LL_HRTIM_TIM_DisablePreload(HRTIM1, LL_HRTIM_TIMER_C);
  LL_HRTIM_TIM_EnableResyncUpdate(HRTIM1, LL_HRTIM_TIMER_C);
  LL_HRTIM_TIM_SetCompare1(HRTIM1, LL_HRTIM_TIMER_C, 614);

  LL_HRTIM_ForceUpdate(HRTIM1, LL_HRTIM_TIMER_C);
  LL_HRTIM_TIM_CounterEnable(HRTIM1, LL_HRTIM_TIMER_C);

  
  // Phase output
  LL_HRTIM_OUT_SetPolarity(HRTIM1, LL_HRTIM_OUTPUT_TA1, LL_HRTIM_OUT_POSITIVE_POLARITY);
  LL_HRTIM_OUT_SetOutputSetSrc(HRTIM1, LL_HRTIM_OUTPUT_TA1, LL_HRTIM_OUTPUTSET_TIMCMP1);
  LL_HRTIM_OUT_SetOutputResetSrc(HRTIM1, LL_HRTIM_OUTPUT_TA1, LL_HRTIM_OUTPUTSET_TIMCMP2);
  LL_HRTIM_OUT_SetIdleMode(HRTIM1, LL_HRTIM_OUTPUT_TA1, LL_HRTIM_OUT_NO_IDLE);
  LL_HRTIM_OUT_SetIdleLevel(HRTIM1, LL_HRTIM_OUTPUT_TA1, LL_HRTIM_OUT_IDLELEVEL_INACTIVE);
  LL_HRTIM_OUT_SetFaultState(HRTIM1, LL_HRTIM_OUTPUT_TA1, LL_HRTIM_OUT_FAULTSTATE_NO_ACTION);
  LL_HRTIM_OUT_SetChopperMode(HRTIM1, LL_HRTIM_OUTPUT_TA1, LL_HRTIM_OUT_CHOPPERMODE_DISABLED);

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_13;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  LL_HRTIM_EnableOutput(HRTIM1, LL_HRTIM_OUTPUT_TA1);

}
#endif