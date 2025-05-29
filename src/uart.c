/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"

void uart1_init(void)
{
    LL_USART_InitTypeDef USART_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    /** USART1 GPIO Configuration
    PA9   ------> USART1_TX
    PA10   ------> USART1_RX */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1_RX DMA Init */
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_USART1_RX);

    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_LOW);

    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MODE_CIRCULAR);

    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);

    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_BYTE);

    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_BYTE);

    /* USART1_TX DMA  Init */
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_2, LL_DMAMUX_REQ_USART1_TX);

    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_LOW);

    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_2, LL_DMA_MODE_NORMAL);

    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);

    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_BYTE);

    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_BYTE);

    USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART1, &USART_InitStruct);
    LL_USART_SetTXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetRXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_DisableFIFO(USART1);
    LL_USART_ConfigAsyncMode(USART1);

    LL_USART_Enable(USART1);

    /* Polling USART1 initialisation */
    while((!(LL_USART_IsActiveFlag_TEACK(USART1))) || (!(LL_USART_IsActiveFlag_REACK(USART1))))
    {
    }
}

void uart1_dma_rx_start(uint32_t *buff, uint32_t len)
{
    LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_1);
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_1, LL_DMAMUX_REQ_USART1_RX);
    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_1, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PRIORITY_HIGH);
    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_1, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_1, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_ConfigAddresses(DMA2, LL_DMA_CHANNEL_1,
                           (uint32_t)&USART1->RDR,
                           (uint32_t)buff,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_1, len);

    LL_DMA_EnableIT_HT(DMA2, LL_DMA_CHANNEL_1);
    LL_DMA_EnableIT_TC(DMA2, LL_DMA_CHANNEL_1);

    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_1);

    LL_USART_EnableDMAReq_RX(USART1);

    NVIC_SetPriority(DMA2_Channel1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
    NVIC_EnableIRQ(DMA2_Channel1_IRQn);
}

void uart1_dma_tx(uint8_t *data, uint32_t len)
{
    if (len == 0) return;

    LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_2);

    LL_DMA_ClearFlag_TC2(DMA2);
    LL_DMA_ClearFlag_TE2(DMA2);

    LL_DMA_ConfigAddresses(DMA2, LL_DMA_CHANNEL_2,
                           (uint32_t)data,
                           LL_USART_DMA_GetRegAddr(USART1, LL_USART_DMA_REG_DATA_TRANSMIT),
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

    LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_2, len);

    LL_USART_EnableDMAReq_TX(USART1);

    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_2);
}
