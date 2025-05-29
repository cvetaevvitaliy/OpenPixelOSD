/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <string.h>
#include "main.h"
#include "msp_displayport.h"
#include "msp.h"
#include "canvas_char.h"

#define USART1_TX_REG_ADDR ((uint32_t)&(USART1->TDR))
#define USART1_RX_REG_ADDR ((uint32_t)&(USART1->RDR))

#define UART_RX_BUF_SIZE (2) //
#define UART_TX_BUF_SIZE (64)
uint8_t uart_tx_buf[UART_TX_BUF_SIZE];
uint8_t uart_rx_buf[UART_RX_BUF_SIZE];

typedef enum {
    MSP_DISPLAYPORT_KEEPALIVE,
    MSP_DISPLAYPORT_RELEASE,
    MSP_DISPLAYPORT_CLEAR,
    MSP_DISPLAYPORT_DRAW_STRING,
    MSP_DISPLAYPORT_DRAW_SCREEN,
    MSP_DISPLAYPORT_SET_OPTIONS,
    MSP_DISPLAYPORT_DRAW_SYSTEM
} msp_displayport_cmd_e;

extern char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
extern uint8_t active_buffer;
extern bool show_logo;

CCMRAM_BSS static msp_port_t msp_uart = {0};
CCMRAM_BSS static uint32_t dma_old_pos = 0;

void uart1_dma_rx_start(uint32_t *buff, uint32_t len);
void uart1_dma_tx(uint8_t *data, uint32_t len);
EXEC_RAM static void msp_callback(msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);

void msp_displayport_init(void)
{
    uart1_init();
    uart1_dma_rx_start((uint32_t*)uart_rx_buf, UART_RX_BUF_SIZE);
    msp_uart.callback = msp_callback;
}

EXEC_RAM static void msp_callback(msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{
    switch(msp_version) {
    case MSP_V1: {
        if (msp_cmd == MSP_DISPLAYPORT) {
            msp_displayport_cmd_e sub_cmd = payload[0];
            switch(sub_cmd) {
            case MSP_DISPLAYPORT_KEEPALIVE: // 0 -> Open/Keep-Alive DisplayPort
            {
                static bool displayport_initialized = false;
                if (!displayport_initialized) {
                    displayport_initialized = true;
                    show_logo = false;
                    // Send canvas size to FC
                    uint8_t data[2] = {COLUMN_SIZE, ROW_SIZE};
                    uint16_t len = construct_msp_command_v1(uart_tx_buf, MSP_SET_OSD_CANVAS, data, 2, MSP_OUTBOUND);
                    uart1_dma_tx(uart_tx_buf, len);
                }
            }
                break;
            case MSP_DISPLAYPORT_RELEASE: // 1 -> Close DisplayPort
                show_logo = true;
                break;
            case MSP_DISPLAYPORT_CLEAR: // 2 -> Clear Screen
                canvas_char_clean();
                break;
            case MSP_DISPLAYPORT_DRAW_STRING:  // 3 -> Draw String
            {
                if (data_size < 5) break;
                uint8_t row = payload[1];
                uint8_t col = payload[2];
                if (row >= ROW_SIZE || col >= COLUMN_SIZE) break;
                uint8_t len = data_size - 4;
                memcpy(&canvas_char_map[active_buffer][row][col], (const char *)&payload[4], len);
            }
                break;
            case MSP_DISPLAYPORT_DRAW_SCREEN: // 4 -> Draw Screen
                canvas_char_draw_complete();
                break;
            case MSP_DISPLAYPORT_SET_OPTIONS: // 5 -> Set Options (HDZero/iNav)
                break;
            default:
                break;
            }
        }
    }
        break;
    case MSP_V2_OVER_V1:
        break;
    case MSP_V2_NATIVE:
        break;
    default:
        break;
    }
}

void DMA2_Channel1_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_HT1(DMA2)) {
        LL_DMA_ClearFlag_HT1(DMA2);
        // Processing the first half of the buffer (0 .. UART_RX_BUF_SIZE/2 - 1)
        for (uint32_t i = 0; i < UART_RX_BUF_SIZE / 2; i++) {
            msp_process_received_data(&msp_uart, uart_rx_buf[i]);
        }
        dma_old_pos = UART_RX_BUF_SIZE / 2;
    }

    if (LL_DMA_IsActiveFlag_TC1(DMA2)) {
        LL_DMA_ClearFlag_TC1(DMA2);
        // Processing the second half of the buffer (UART_RX_BUF_SIZE/2 .. UART_RX_BUF_SIZE - 1)
        for (uint32_t i = UART_RX_BUF_SIZE / 2; i < UART_RX_BUF_SIZE; i++) {
            msp_process_received_data(&msp_uart, uart_rx_buf[i]);
        }
        dma_old_pos = 0;
    }
}

void DMA2_Channel2_IRQHandler(void)
{
    if (LL_DMA_IsActiveFlag_TC2(DMA2))
    {
        LL_DMA_ClearFlag_TC2(DMA2);
    }
    if (LL_DMA_IsActiveFlag_TE2(DMA2))
    {
        LL_DMA_ClearFlag_TE2(DMA2);
    }
}
