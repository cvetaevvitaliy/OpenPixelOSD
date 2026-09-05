/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"
#include <stdbool.h>
#include <string.h>


#define UART_RX_RING_BUF_SIZE 1024
#define UART_TX_BUF_SIZE 512
#define UART_RX_DMA_BUF_SIZE (2)
static uint8_t uart_rx_ring_buff[UART_RX_RING_BUF_SIZE];
static volatile uint16_t uart_rx_tail = 0;
static uint8_t uart_tx_buf[UART_TX_BUF_SIZE];
static volatile bool tx_bysy = false;

#define UART3_RX_RING_BUF_SIZE 64
#define UART3_TX_BUF_SIZE 64
#define UART3_RX_DMA_BUF_SIZE (2)
static uint8_t uart3_rx_ring_buff[UART3_RX_RING_BUF_SIZE];
static volatile uint16_t uart3_rx_tail = 0;
static uint8_t uart3_tx_buf[UART3_TX_BUF_SIZE];
static volatile bool tx3_bysy = false;

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
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_3, LL_DMAMUX_REQ_USART1_RX);

    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_3, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PRIORITY_LOW);

    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MODE_CIRCULAR);

    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PERIPH_NOINCREMENT);

    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MEMORY_INCREMENT);

    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PDATAALIGN_BYTE);

    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MDATAALIGN_BYTE);

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

    NVIC_SetPriority(DMA2_Channel2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 0));
    NVIC_EnableIRQ(DMA2_Channel2_IRQn);

    LL_USART_EnableIT_ERROR(USART1); 
    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
    NVIC_EnableIRQ(USART1_IRQn);

}

void uart1_dma_rx_start(void)
{
    LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_3);
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_3, LL_DMAMUX_REQ_USART1_RX);
    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_3, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PRIORITY_HIGH);
    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_ConfigAddresses(DMA2, LL_DMA_CHANNEL_3,
                           (uint32_t)&USART1->RDR,
                           (uint32_t)uart_rx_ring_buff,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_3, UART_RX_RING_BUF_SIZE);


    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_3);

    LL_USART_EnableDMAReq_RX(USART1);

}

void uart1_tx_dma(uint8_t *data, uint32_t len)
{
    while(tx_bysy){};
    if (len == 0) return;
    if (len > UART_TX_BUF_SIZE) len = UART_TX_BUF_SIZE;
    memcpy(uart_tx_buf, data, len);
    tx_bysy = true;

    LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_2);

    LL_DMA_ClearFlag_TC2(DMA2);
    LL_DMA_ClearFlag_TE2(DMA2);

    LL_DMA_ConfigAddresses(DMA2, LL_DMA_CHANNEL_2,
                           (uint32_t)uart_tx_buf,
                           LL_USART_DMA_GetRegAddr(USART1, LL_USART_DMA_REG_DATA_TRANSMIT),
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

    LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_2, len);

    LL_USART_EnableDMAReq_TX(USART1);

    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_2);

    LL_DMA_EnableIT_TE(DMA2, LL_DMA_CHANNEL_2);
    LL_DMA_EnableIT_TC(DMA2, LL_DMA_CHANNEL_2);

    LL_USART_EnableDMAReq_TX(USART1);
    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_2);
}

EXEC_RAM void DMA2_Channel2_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC2(DMA2))
    {
        LL_DMA_ClearFlag_TC2(DMA2);
        while (!LL_USART_IsActiveFlag_TC(USART1)) {}
        LL_USART_DisableDMAReq_TX(USART1);
        tx_bysy = false;
    }
    if (LL_DMA_IsActiveFlag_TE2(DMA2))
    {
        LL_DMA_ClearFlag_TE2(DMA2);
        tx_bysy = false;
        TRACE_ERROR("DMA UART TX Error\n");
    }
}

EXEC_RAM bool uart1_rx_ring_get(uint8_t *data)
{
    uint16_t uart_rx_head = UART_RX_RING_BUF_SIZE - LL_DMA_GetDataLength(DMA2, LL_DMA_CHANNEL_3);
    if (uart_rx_head == uart_rx_tail) return false;

    *data = uart_rx_ring_buff[uart_rx_tail];
    uart_rx_tail = (uart_rx_tail + 1) % UART_RX_RING_BUF_SIZE;
    return true;
}

void USART1_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_ORE(USART1)) {
        LL_USART_ClearFlag_ORE(USART1);
        TRACE_ERROR("Overrun Error\n");
    }

    if (LL_USART_IsActiveFlag_FE(USART1)) {
        LL_USART_ClearFlag_FE(USART1);
        //TRACE_ERROR("Framing Error\n");
    }

    if (LL_USART_IsActiveFlag_NE(USART1)) {
        LL_USART_ClearFlag_NE(USART1);
        TRACE_ERROR("Noise Error\n");
    }

    if (LL_USART_IsActiveFlag_PE(USART1)) {
        LL_USART_ClearFlag_PE(USART1);
        TRACE_ERROR("Parity Error\n");
    }
}



void uart3_init(void)
{
    LL_USART_InitTypeDef USART_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_RCC_SetUSARTClockSource(LL_RCC_USART3_CLKSOURCE_PCLK1);

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    /**USART3 GPIO Configuration
    PC10   ------> USART3_TX
    PC11   ------> USART3_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USART1_RX DMA Init */
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_3, LL_DMAMUX_REQ_USART1_RX);
    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_3, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_3, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_3, LL_DMA_MDATAALIGN_BYTE);

    /* USART1_TX DMA  Init */
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_4, LL_DMAMUX_REQ_USART3_TX);
    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_4, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_4, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_4, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_4, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_4, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_4, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_4, LL_DMA_MDATAALIGN_BYTE);

    USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART3, &USART_InitStruct);
    LL_USART_SetTXFIFOThreshold(USART3, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_SetRXFIFOThreshold(USART3, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_DisableFIFO(USART3);
    LL_USART_ConfigAsyncMode(USART3);

    LL_USART_Enable(USART3);

    /* Polling USART1 initialisation */
    while((!(LL_USART_IsActiveFlag_TEACK(USART3))) || (!(LL_USART_IsActiveFlag_REACK(USART3))))
    {
    }

    NVIC_SetPriority(DMA2_Channel4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 0));
    NVIC_EnableIRQ(DMA2_Channel4_IRQn);

    LL_USART_EnableIT_ERROR(USART3); 
    NVIC_SetPriority(USART3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
    NVIC_EnableIRQ(USART3_IRQn);
}

void uart3_dma_rx_start(void)
{
    LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_6);
    LL_DMA_SetPeriphRequest(DMA2, LL_DMA_CHANNEL_6, LL_DMAMUX_REQ_USART3_RX);
    LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA2, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_HIGH);
    LL_DMA_SetMode(DMA2, LL_DMA_CHANNEL_6, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA2, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA2, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_BYTE);
    LL_DMA_ConfigAddresses(DMA2, LL_DMA_CHANNEL_6,
                           (uint32_t)&USART3->RDR,
                           (uint32_t)uart3_rx_ring_buff,
                           LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_6, UART3_RX_RING_BUF_SIZE);

    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_6);

    LL_USART_EnableDMAReq_RX(USART3);

    NVIC_SetPriority(DMA2_Channel6_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2, 0));
    NVIC_EnableIRQ(DMA2_Channel6_IRQn);
}

void uart3_tx_dma(uint8_t *data, uint32_t len)
{
    while(tx3_bysy){};
    if (len == 0) return;
    if (len > UART3_TX_BUF_SIZE) len = UART3_TX_BUF_SIZE;
    memcpy(uart3_tx_buf, data, len);
    tx3_bysy = true;

    LL_DMA_DisableChannel(DMA2, LL_DMA_CHANNEL_4);

    LL_DMA_ClearFlag_TC4(DMA2);
    LL_DMA_ClearFlag_TE4(DMA2);

    LL_DMA_ConfigAddresses(DMA2, LL_DMA_CHANNEL_4,
                           (uint32_t)uart3_tx_buf,
                           LL_USART_DMA_GetRegAddr(USART3, LL_USART_DMA_REG_DATA_TRANSMIT),
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

    LL_DMA_SetDataLength(DMA2, LL_DMA_CHANNEL_4, len);

    LL_USART_EnableDMAReq_TX(USART3);

    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_4);

    LL_DMA_EnableIT_TE(DMA2, LL_DMA_CHANNEL_4);
    LL_DMA_EnableIT_TC(DMA2, LL_DMA_CHANNEL_4);

    LL_USART_EnableDMAReq_TX(USART3);
    LL_DMA_EnableChannel(DMA2, LL_DMA_CHANNEL_4);
}

void DMA2_Channel4_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC4(DMA2))
    {
        LL_DMA_ClearFlag_TC4(DMA2);
        while (!LL_USART_IsActiveFlag_TC(USART3)) {}
        LL_USART_DisableDMAReq_TX(USART3);
        tx3_bysy = false;
    }
    if (LL_DMA_IsActiveFlag_TE4(DMA2))
    {
        LL_DMA_ClearFlag_TE4(DMA2);
        tx3_bysy = false;
        TRACE_ERROR("DMA UART3 TX Error\n");
    }
}

bool uart3_rx_ring_get(uint8_t *data)
{
    uint16_t uart3_rx_head = UART3_RX_RING_BUF_SIZE - LL_DMA_GetDataLength(DMA2, LL_DMA_CHANNEL_6);
    if (uart3_rx_head == uart3_rx_tail) return false;

    *data = uart3_rx_ring_buff[uart3_rx_tail];
    uart3_rx_tail = (uart3_rx_tail + 1) % UART3_RX_RING_BUF_SIZE;
    return true;
}

void USART3_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_ORE(USART3)) {
        LL_USART_ClearFlag_ORE(USART3);
        TRACE_ERROR("USART3 Overrun Error\n");
    }

    if (LL_USART_IsActiveFlag_FE(USART3)) {
        LL_USART_ClearFlag_FE(USART3);
        //TRACE_ERROR("Framing Error\n");
    }

    if (LL_USART_IsActiveFlag_NE(USART3)) {
        LL_USART_ClearFlag_NE(USART3);
        TRACE_ERROR("USART3 Noise Error\n");
    }

    if (LL_USART_IsActiveFlag_PE(USART3)) {
        LL_USART_ClearFlag_PE(USART3);
        TRACE_ERROR("USART3 Parity Error\n");
    }
}