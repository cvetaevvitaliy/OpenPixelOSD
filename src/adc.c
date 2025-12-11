/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"

/* If your VDDA is not 3.3 V, override this (in mV) for better temp approximation */
#ifndef ADC_VDDA_ASSUMED_mV
#define ADC_VDDA_ASSUMED_mV         3300u
#endif

#define USE_VREF_IN_CHANNEL         1

static volatile uint16_t g_adc_dma_buf[ADC_CH_COUNT];

void adc_init(void)
{
    LL_ADC_InitTypeDef ADC_InitStruct = {0};
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
    LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};
    LL_ADC_INJ_InitTypeDef ADC_INJ_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_RCC_SetADCClockSource(LL_RCC_ADC12_CLKSOURCE_SYSCLK);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC12);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    /**ADC1 GPIO Configuration
        PB11   ------> ADC1_IN14
        PB12   ------> ADC1_IN11
    */
    GPIO_InitStruct.Pin = ADC_RESERVED_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(ADC_RESERVED_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = ADC_PA_VDET_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(ADC_PA_VDET_GPIO_Port, &GPIO_InitStruct);

    /* ADC1 DMA Init */

    /* ADC1 Init */
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_4, LL_DMAMUX_REQ_ADC1);

    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_4, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PRIORITY_LOW);

    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MODE_CIRCULAR);

    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PERIPH_NOINCREMENT);

    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PDATAALIGN_HALFWORD);

    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MDATAALIGN_HALFWORD);

    LL_DMA_SetPeriphAddress        (DMA1, LL_DMA_CHANNEL_4, (uint32_t)&ADC1->DR);
    LL_DMA_SetMemoryAddress        (DMA1, LL_DMA_CHANNEL_4, (uint32_t)g_adc_dma_buf);
    LL_DMA_SetDataLength           (DMA1, LL_DMA_CHANNEL_4, ADC_CH_COUNT);

    /** Common config */
    ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
    LL_ADC_Init(ADC1, &ADC_InitStruct);
    ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
    ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS;
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS;
    ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_UNLIMITED;
    ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
    LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
    LL_ADC_SetGainCompensation(ADC1, 0);
    LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);
    ADC_CommonInitStruct.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV4;
    ADC_CommonInitStruct.Multimode = LL_ADC_MULTI_INDEPENDENT;
    LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &ADC_CommonInitStruct);
    ADC_INJ_InitStruct.TriggerSource = LL_ADC_INJ_TRIG_SOFTWARE;
    ADC_INJ_InitStruct.SequencerLength = LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS;
    ADC_INJ_InitStruct.SequencerDiscont = LL_ADC_INJ_SEQ_DISCONT_DISABLE;
    ADC_INJ_InitStruct.TrigAuto = LL_ADC_INJ_TRIG_FROM_GRP_REGULAR;
    LL_ADC_INJ_Init(ADC1, &ADC_INJ_InitStruct);
    LL_ADC_INJ_SetQueueMode(ADC1, LL_ADC_INJ_QUEUE_DISABLE);

    /* Disable ADC deep power down (enabled by default after reset state) */
    LL_ADC_DisableDeepPowerDown(ADC1);
    /* Enable ADC internal voltage regulator */
    LL_ADC_EnableInternalRegulator(ADC1);
    /* Delay for ADC internal voltage regulator stabilization. */
    /* Compute number of CPU cycles to wait for, from delay in us. */
    /* Note: Variable divided by 2 to compensate partially */
    /* CPU processing cycles (depends on compilation optimization). */
    /* Note: If system core clock frequency is below 200kHz, wait time */
    /* is only a few CPU processing cycles. */
    uint32_t wait_loop_index;
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while(wait_loop_index != 0)
    {
        wait_loop_index--;
    }

    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS);

    // sync with `adc_ch_t` in `main.h`, RANK determines order in ADC DMA buffer and `adc_ch_t` is used to index into the buffer.
    // see also: `adc_read_raw()`

    /** Configure Regular Channel */
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, ADC_RESERVED_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_RESERVED_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_RESERVED_Channel, LL_ADC_SINGLE_ENDED);

    /** Configure Injected Channel */
    LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_1, ADC_RESERVED_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_RESERVED_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_RESERVED_Channel, LL_ADC_SINGLE_ENDED);

    /** Configure Regular Channel */
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_2, ADC_PA_VDET_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_PA_VDET_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_PA_VDET_Channel, LL_ADC_SINGLE_ENDED);

    /** Configure Injected Channel */
    LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_2, ADC_PA_VDET_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_PA_VDET_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_PA_VDET_Channel, LL_ADC_SINGLE_ENDED);

    /** Configure Regular Channel */
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_3, LL_ADC_CHANNEL_TEMPSENSOR_ADC1);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SAMPLINGTIME_247CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SINGLE_ENDED);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_TEMPSENSOR | LL_ADC_PATH_INTERNAL_VREFINT);

    /** Configure Injected Channel */
    LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_3, LL_ADC_CHANNEL_TEMPSENSOR_ADC1);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SAMPLINGTIME_247CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SINGLE_ENDED);

    /** Configure Regular Channel */
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_4, LL_ADC_CHANNEL_VREFINT);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SAMPLINGTIME_247CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SINGLE_ENDED);

    /** Configure Injected Channel */
    LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_4, LL_ADC_CHANNEL_VREFINT);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SAMPLINGTIME_247CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SINGLE_ENDED);


    NVIC_SetPriority(DMA1_Channel4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);

    /* Make sure ADC is disabled before configuration */
    if (LL_ADC_IsEnabled(ADC1)) {
        LL_ADC_Disable(ADC1);
        while (LL_ADC_IsEnabled(ADC1)) { /* wait */ }
    }

    /* Enable ADC internal regulator and wait t ADCVREG_STUP (~20 µs) */
    LL_ADC_EnableInternalRegulator(ADC1);
    for (volatile uint32_t i = 0; i < 2000; ++i) { __NOP(); } // crude ~>20 µs

    /* Calibrate (single-ended) */
    LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC1)) { /* wait */ }
    for (volatile uint32_t i = 0; i < 2000; ++i) { __NOP(); } // stabilization

    /* Enable DMA */
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);

    /* Enable ADC and wait ready */
    LL_ADC_Enable(ADC1);
    while (!LL_ADC_IsActiveFlag_ADRDY(ADC1)) { /* wait */ }

    /* Temp sensor stabilization (>10 µs) */
    for (volatile uint32_t i = 0; i < 2000; ++i) { __NOP(); }

    LL_ADC_REG_StartConversion(ADC1);
}

uint16_t adc_read_raw(adc_ch_t ch)
{
    if ((uint32_t)ch >= ADC_CH_COUNT) return 0;
    return g_adc_dma_buf[ch];
}

uint16_t adc_read_mv(adc_ch_t ch)
{
    return (uint16_t)DAC12BIT_TO_MV(adc_read_raw(ch));
}

uint32_t adc_read_vdda_mv(void)
{
    const uint16_t vref_cal = *VREFINT_CAL_ADDR;
    const uint16_t vref_raw = adc_read_raw(ADC_CH_VREF_INT);
    if (vref_raw == 0) return 3300u;
    // VDDA ≈ 3000mV * VREFINT_CAL / VREFINT_NOW
    return (uint32_t)3000u * (uint32_t)vref_cal / (uint32_t)vref_raw;
}

float adc_read_mcu_temp_c(void)
{
    const uint16_t ts_raw = adc_read_raw(ADC_CH_TEMP);
#if USE_VREF_IN_CHANNEL
    const uint32_t vdda_mv = adc_read_vdda_mv();
    const int32_t t = __LL_ADC_CALC_TEMPERATURE(vdda_mv, ts_raw, LL_ADC_RESOLUTION_12B);
#else
    const int32_t t = __LL_ADC_CALC_TEMPERATURE(ADC_VDDA_ASSUMED_mV, ts_raw, LL_ADC_RESOLUTION_12B);
#endif

    return (float)t;
}