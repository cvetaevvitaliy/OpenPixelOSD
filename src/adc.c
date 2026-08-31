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

#define ADC_RANK_TABLE_LEN 4
static const uint32_t g_adc_reg_rank[ADC_RANK_TABLE_LEN] = {
    LL_ADC_REG_RANK_1, LL_ADC_REG_RANK_2, LL_ADC_REG_RANK_3, LL_ADC_REG_RANK_4,
};
static const uint32_t g_adc_inj_rank[ADC_RANK_TABLE_LEN] = {
    LL_ADC_INJ_RANK_1, LL_ADC_INJ_RANK_2, LL_ADC_INJ_RANK_3, LL_ADC_INJ_RANK_4,
};
static const uint32_t g_adc_reg_seq_len[ADC_RANK_TABLE_LEN] = {
    LL_ADC_REG_SEQ_SCAN_DISABLE,        // 1 channel
    LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS,
    LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS,
    LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS,
};
static const uint32_t g_adc_inj_seq_len[ADC_RANK_TABLE_LEN] = {
    LL_ADC_INJ_SEQ_SCAN_DISABLE,        // 1 channel
    LL_ADC_INJ_SEQ_SCAN_ENABLE_2RANKS,
    LL_ADC_INJ_SEQ_SCAN_ENABLE_3RANKS,
    LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS,
};

static volatile uint16_t g_adc1_dma_buf[ADC1_CH_COUNT];
#if defined(ADC2_NEEDED)
static volatile uint16_t g_adc2_dma_buf[ADC2_CH_COUNT];
#endif

uint16_t adc1_read_raw(adc1_ch_t ch)
{
    if ((uint32_t)ch >= ADC1_CH_COUNT) return 0;
    return g_adc1_dma_buf[ch];
}

uint16_t adc1_read_mv(adc1_ch_t ch)
{
    return (uint16_t)DAC12BIT_TO_MV(adc1_read_raw(ch));
}

#if defined(ADC2_NEEDED)
uint16_t adc2_read_raw(adc2_ch_t ch)
{
    if ((uint32_t)ch >= ADC2_CH_COUNT) return 0;
    return g_adc2_dma_buf[ch];
}

uint16_t adc2_read_mv(adc2_ch_t ch)
{
    return (uint16_t)DAC12BIT_TO_MV(adc2_read_raw(ch));
}

/* ADC2, free-running via its own DMA channel, independent of ADC1's
 * sequence below. Registers whichever of RESERVED/NTC/PA_VDET actually
 * claimed ADC2 on this target */
static void adc2_init(void)
{
    LL_ADC_InitTypeDef ADC_InitStruct = {0};
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
    LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};

    uint8_t adc2_ch_count = 0;
#if defined(ADC_RESERVED_INSTANCE) && ADC_RESERVED_INSTANCE == ADC_INSTANCE_2
    adc2_ch_count++;
#endif
#if defined(ADC_NTC_INSTANCE) && ADC_NTC_INSTANCE == ADC_INSTANCE_2
    adc2_ch_count++;
#endif
#if defined(ADC_PA_VDET_INSTANCE) && ADC_PA_VDET_INSTANCE == ADC_INSTANCE_2
    adc2_ch_count++;
#endif

    /* DMA1_Channel5 -- confirmed free: DMA1 channels 1/2/6 are used by
     * tim.c/dac.c/video_gen.c, DMA1_Channel3 by dma.c (mem2mem), DMA1_4
     * by ADC1 below, DMA2_1/2 by uart.c. */
    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_5, LL_DMAMUX_REQ_ADC2);
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_5, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_5, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_5, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_5, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_5, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_5, LL_DMA_PDATAALIGN_HALFWORD);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_5, LL_DMA_MDATAALIGN_HALFWORD);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&ADC2->DR);
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)g_adc2_dma_buf);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, adc2_ch_count);

    ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
    LL_ADC_Init(ADC2, &ADC_InitStruct);

    ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
    ADC_REG_InitStruct.SequencerLength = g_adc_reg_seq_len[adc2_ch_count - 1];
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS;
    ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_UNLIMITED;
    ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
    LL_ADC_REG_Init(ADC2, &ADC_REG_InitStruct);
    LL_ADC_SetGainCompensation(ADC2, 0);
    LL_ADC_SetOverSamplingScope(ADC2, LL_ADC_OVS_DISABLE);

    ADC_CommonInitStruct.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV4;
    ADC_CommonInitStruct.Multimode = LL_ADC_MULTI_INDEPENDENT;
    LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC2), &ADC_CommonInitStruct);

    LL_ADC_REG_SetSequencerLength(ADC2, g_adc_reg_seq_len[adc2_ch_count - 1]);

    uint8_t adc2_rank_idx = 0;

#if defined(ADC_RESERVED_INSTANCE) && ADC_RESERVED_INSTANCE == ADC_INSTANCE_2
    LL_ADC_REG_SetSequencerRanks(ADC2, g_adc_reg_rank[adc2_rank_idx], ADC_RESERVED_Channel);
    LL_ADC_SetChannelSamplingTime(ADC2, ADC_RESERVED_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC2, ADC_RESERVED_Channel, LL_ADC_SINGLE_ENDED);
    adc2_rank_idx++;
#endif

#if defined(ADC_NTC_INSTANCE) && ADC_NTC_INSTANCE == ADC_INSTANCE_2
    LL_ADC_REG_SetSequencerRanks(ADC2, g_adc_reg_rank[adc2_rank_idx], ADC_NTC_Channel);
    LL_ADC_SetChannelSamplingTime(ADC2, ADC_NTC_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC2, ADC_NTC_Channel, LL_ADC_SINGLE_ENDED);
    adc2_rank_idx++;
#endif

#if defined(ADC_PA_VDET_INSTANCE) && ADC_PA_VDET_INSTANCE == ADC_INSTANCE_2
    LL_ADC_REG_SetSequencerRanks(ADC2, g_adc_reg_rank[adc2_rank_idx], ADC_PA_VDET_Channel);
    LL_ADC_SetChannelSamplingTime(ADC2, ADC_PA_VDET_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC2, ADC_PA_VDET_Channel, LL_ADC_SINGLE_ENDED);
    adc2_rank_idx++;
#endif

    NVIC_SetPriority(DMA1_Channel5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);

    if (LL_ADC_IsEnabled(ADC2)) {
        LL_ADC_Disable(ADC2);
        while (LL_ADC_IsEnabled(ADC2)) { /* wait */ }
    }

    LL_ADC_DisableDeepPowerDown(ADC2); // was missing -- ADC2 defaults to deep-power-down on reset same as ADC1; without clearing it first, EnableInternalRegulator/calibration can appear to complete (ADRDY sets, no hang) while the analog front-end never actually powers up, leaving every conversion reading 0
    LL_ADC_EnableInternalRegulator(ADC2);
    for (volatile uint32_t i = 0; i < 2000; ++i) { __NOP(); } // crude ~>20 us

    LL_ADC_StartCalibration(ADC2, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC2)) { /* wait */ }
    for (volatile uint32_t i = 0; i < 2000; ++i) { __NOP(); }

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);

    LL_ADC_Enable(ADC2);
    while (!LL_ADC_IsActiveFlag_ADRDY(ADC2)) { /* wait */ }

    LL_ADC_REG_StartConversion(ADC2);
}

void adc2_vdet_debug_status(bool *adc_enabled, bool *adc_ready, bool *dma_enabled, uint16_t *dma_remaining)
{
    *adc_enabled = LL_ADC_IsEnabled(ADC2);
    *adc_ready = LL_ADC_IsActiveFlag_ADRDY(ADC2);
    *dma_enabled = LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5);
    *dma_remaining = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_5);
}
#endif // ADC2_NEEDED

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
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

#if defined(ADC_RESERVED_INSTANCE)
    GPIO_InitStruct.Pin = ADC_RESERVED_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(ADC_RESERVED_GPIO_Port, &GPIO_InitStruct);
#endif

#if defined(ADC_NTC_INSTANCE)
    GPIO_InitStruct.Pin = ADC_NTC_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(ADC_NTC_GPIO_Port, &GPIO_InitStruct);
#endif

#if defined(ADC_PA_VDET_INSTANCE)
    GPIO_InitStruct.Pin = ADC_PA_VDET_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(ADC_PA_VDET_GPIO_Port, &GPIO_InitStruct);
#endif

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
    LL_DMA_SetMemoryAddress        (DMA1, LL_DMA_CHANNEL_4, (uint32_t)g_adc1_dma_buf);
    LL_DMA_SetDataLength           (DMA1, LL_DMA_CHANNEL_4, ADC1_CH_COUNT);

    /** Common config */
    ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
    LL_ADC_Init(ADC1, &ADC_InitStruct);
    ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
    ADC_REG_InitStruct.SequencerLength = g_adc_reg_seq_len[ADC1_CH_COUNT - 1];
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
    ADC_INJ_InitStruct.SequencerLength = g_adc_inj_seq_len[ADC1_CH_COUNT - 1];
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

    LL_ADC_REG_SetSequencerLength(ADC1, g_adc_reg_seq_len[ADC1_CH_COUNT - 1]);

    // Rank order below determines g_adc1_dma_buf[] layout, which MUST
    // match adc1_ch_t's own member order in the target header exactly
    // (RESERVED, then NTC, then PA_VDET -- whichever of those actually
    // claimed ADC1 -- then TEMP, then VREF_INT), since adc1_read_raw()
    // indexes that buffer with the enum value directly, no separate
    // offset table. adc1_rank_idx tracks how many channels have
    // actually been registered so far, so each one gets the next rank
    // in sequence with nothing hardcoded.
    uint8_t adc1_rank_idx = 0;

#if defined(ADC_RESERVED_INSTANCE) && ADC_RESERVED_INSTANCE == ADC_INSTANCE_1
    LL_ADC_REG_SetSequencerRanks(ADC1, g_adc_reg_rank[adc1_rank_idx], ADC_RESERVED_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_RESERVED_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_RESERVED_Channel, LL_ADC_SINGLE_ENDED);

    LL_ADC_INJ_SetSequencerRanks(ADC1, g_adc_inj_rank[adc1_rank_idx], ADC_RESERVED_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_RESERVED_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_RESERVED_Channel, LL_ADC_SINGLE_ENDED);
    adc1_rank_idx++;
#endif

#if defined(ADC_NTC_INSTANCE) && ADC_NTC_INSTANCE == ADC_INSTANCE_1
    LL_ADC_REG_SetSequencerRanks(ADC1, g_adc_reg_rank[adc1_rank_idx], ADC_NTC_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_NTC_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_NTC_Channel, LL_ADC_SINGLE_ENDED);

    LL_ADC_INJ_SetSequencerRanks(ADC1, g_adc_inj_rank[adc1_rank_idx], ADC_NTC_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_NTC_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_NTC_Channel, LL_ADC_SINGLE_ENDED);
    adc1_rank_idx++;
#endif

#if defined(ADC_PA_VDET_INSTANCE) && ADC_PA_VDET_INSTANCE == ADC_INSTANCE_1
    LL_ADC_REG_SetSequencerRanks(ADC1, g_adc_reg_rank[adc1_rank_idx], ADC_PA_VDET_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_PA_VDET_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_PA_VDET_Channel, LL_ADC_SINGLE_ENDED);

    LL_ADC_INJ_SetSequencerRanks(ADC1, g_adc_inj_rank[adc1_rank_idx], ADC_PA_VDET_Channel);
    LL_ADC_SetChannelSamplingTime(ADC1, ADC_PA_VDET_Channel, LL_ADC_SAMPLINGTIME_92CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, ADC_PA_VDET_Channel, LL_ADC_SINGLE_ENDED);
    adc1_rank_idx++;
#endif

    /** Configure Regular Channel */
    LL_ADC_REG_SetSequencerRanks(ADC1, g_adc_reg_rank[adc1_rank_idx], LL_ADC_CHANNEL_TEMPSENSOR_ADC1);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SAMPLINGTIME_247CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SINGLE_ENDED);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_TEMPSENSOR | LL_ADC_PATH_INTERNAL_VREFINT);

    /** Configure Injected Channel */
    LL_ADC_INJ_SetSequencerRanks(ADC1, g_adc_inj_rank[adc1_rank_idx], LL_ADC_CHANNEL_TEMPSENSOR_ADC1);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SAMPLINGTIME_247CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SINGLE_ENDED);
    adc1_rank_idx++;

    /** Configure Regular Channel */
    LL_ADC_REG_SetSequencerRanks(ADC1, g_adc_reg_rank[adc1_rank_idx], LL_ADC_CHANNEL_VREFINT);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SAMPLINGTIME_247CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SINGLE_ENDED);

    /** Configure Injected Channel */
    LL_ADC_INJ_SetSequencerRanks(ADC1, g_adc_inj_rank[adc1_rank_idx], LL_ADC_CHANNEL_VREFINT);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SAMPLINGTIME_247CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_VREFINT, LL_ADC_SINGLE_ENDED);
    adc1_rank_idx++;


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

#if defined(ADC2_NEEDED)
    adc2_init();
#endif
}

uint32_t adc_read_vdda_mv(void)
{
    const uint16_t vref_cal = *VREFINT_CAL_ADDR;
    const uint16_t vref_raw = adc1_read_raw(ADC1_CH_VREF_INT);
    if (vref_raw == 0) return 3300u;
    // VDDA ≈ 3000mV * VREFINT_CAL / VREFINT_NOW
    return (uint32_t)3000u * (uint32_t)vref_cal / (uint32_t)vref_raw;
}

#define __LL_ADC_CALC_TEMPERATURE_DECIDEGREES(__VREFANALOG_VOLTAGE__,\
                                              __TEMPSENSOR_ADC_DATA__,\
                                              __ADC_RESOLUTION__)\
((((int32_t)*TEMPSENSOR_CAL2_ADDR - (int32_t)*TEMPSENSOR_CAL1_ADDR) != 0) ?       \
 (((( ((int32_t)((__LL_ADC_CONVERT_DATA_RESOLUTION((__TEMPSENSOR_ADC_DATA__),     \
                                                   (__ADC_RESOLUTION__),          \
                                                   LL_ADC_RESOLUTION_12B)         \
                  * (__VREFANALOG_VOLTAGE__))                                     \
                 / TEMPSENSOR_CAL_VREFANALOG)                                     \
       - (int32_t) *TEMPSENSOR_CAL1_ADDR)                                         \
    ) * (int32_t)(TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP) * 10               \
   ) / (int32_t)((int32_t)*TEMPSENSOR_CAL2_ADDR - (int32_t)*TEMPSENSOR_CAL1_ADDR) \
  ) + (TEMPSENSOR_CAL1_TEMP * 10)                                                 \
 )                                                                                \
 :                                                                                \
 ((int32_t)LL_ADC_TEMPERATURE_CALC_ERROR)                                         \
)

/* __LL_ADC_CALC_TEMPERATURE (used previously here) returns whole-degree
 * Celsius only.
 *
 * This replicates the same linear-interpolation formula between the two factory
 * calibration points (TEMPSENSOR_CAL1_ADDR/CAL2_ADDR, at
 * TEMPSENSOR_CAL1_TEMP/CAL2_TEMP) that the macro itself uses internally,
 * but carries one extra digit of precision through the division instead
 * of losing it, so the result is in tenths of a degree C (deci-degrees)
 */
float adc_read_mcu_temp_c(void)
{
    const uint16_t ts_raw = adc1_read_raw(ADC1_CH_TEMP);
#if USE_VREF_IN_CHANNEL
    const uint32_t vdda_mv = adc_read_vdda_mv();
#else
    const uint32_t vdda_mv = ADC_VDDA_ASSUMED_mV;
#endif
    const int32_t t_x10 = __LL_ADC_CALC_TEMPERATURE_DECIDEGREES(vdda_mv, ts_raw, LL_ADC_RESOLUTION_12B);

    return (float)t_x10 / 10.0f;
}
