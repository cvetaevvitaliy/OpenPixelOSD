/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include "msp_displayport.h"
#include "canvas_char.h"
#include "fonts/update_font.h"
#include "main.h"
#include "msp.h"
#include "hardware/uart.h"
#include "hardware/usb.h"
#include "video_overlay.h"


typedef enum {
    MSP_DISPLAYPORT_KEEPALIVE,
    MSP_DISPLAYPORT_RELEASE,
    MSP_DISPLAYPORT_CLEAR,
    MSP_DISPLAYPORT_DRAW_STRING,
    MSP_DISPLAYPORT_DRAW_SCREEN,
    MSP_DISPLAYPORT_SET_OPTIONS,
    MSP_DISPLAYPORT_DRAW_SYSTEM
} msp_displayport_cmd_t;

extern char canvas_char_map[2][ROW_SIZE][COLUMN_SIZE];
extern uint8_t active_buffer;
extern bool show_logo;

CCMRAM_BSS static msp_port_t msp_uart = {0};
CCMRAM_BSS static msp_port_t msp_usb = {0};
EXEC_RAM static void msp_callback(uint8_t owner, msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);

void msp_displayport_init(void)
{
    uart1_init();
    uart1_dma_rx_start();
    msp_uart.callback = msp_callback;
    msp_uart.owner = MSP_OWNER_UART;

    msp_usb.callback = msp_callback;
    msp_usb.owner = MSP_OWNER_USB;
}

EXEC_RAM static void msp_callback(uint8_t owner, msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{
    switch(msp_version) {
    case MSP_V1: {
        switch(msp_cmd) {
        case MSP_DISPLAYPORT: {
            if (data_size == 0) break;
            msp_displayport_cmd_t sub_cmd = payload[0];
            switch(sub_cmd) {
            case MSP_DISPLAYPORT_KEEPALIVE: // 0 -> Open/Keep-Alive DisplayPort
            {
                static bool displayport_initialized = false;
                if (!displayport_initialized) {
                    displayport_initialized = true;
                    // Send canvas size to FC
                    uint8_t data[2] = {COLUMN_SIZE, ROW_SIZE};
                    uint8_t tx_buff[64];
                    uint16_t len = construct_msp_command_v1(tx_buff, MSP_SET_OSD_CANVAS, data, 2, MSP_OUTBOUND);
                    switch(owner) {
                    case MSP_OWNER_UART:
                        uart1_tx_dma(tx_buff, len);
                        break;
                    case MSP_OWNER_USB:
                        usb_uart_write_bytes((const char *)tx_buff, len);
                        break;
                    default:
                        break;
                    }
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
                uint16_t len = data_size - 4;
                if (len > COLUMN_SIZE - col) len = COLUMN_SIZE - col;
                memcpy(&canvas_char_map[active_buffer][row][col], (const char *)&payload[4], len);
            }
                break;
            case MSP_DISPLAYPORT_DRAW_SCREEN: // 4 -> Draw Screen
                show_logo = false;
                canvas_char_draw_complete();
                break;
            case MSP_DISPLAYPORT_SET_OPTIONS: // 5 -> Set Options (HDZero/iNav)
                break;
            default:
                break;
            }
        }
            break;

        case  MSP_OSD_CHAR_WRITE: {
            update_font_symbol_write(payload[0], &payload[1], data_size - 1);
        }
            break;

        case MSP_DEBUG: {
            // Standard four-word debug response; does not change OSD state.
            video_diagnostics_t status;
            video_get_diagnostics(&status);
            uint16_t words[4] = {
                status.threshold_mv | ((uint16_t)status.input << 14) |
                    (status.locked ? 0x8000U : 0) | (status.dma_errors ? 0x2000U : 0),
                (uint16_t)status.captures,
                (uint16_t)status.armed_lines,
                (uint16_t)status.completed_lines
            };
            uint8_t payload_out[8];
            for (unsigned i = 0; i < 4; i++) {
                payload_out[2 * i] = words[i] & 0xff;
                payload_out[2 * i + 1] = words[i] >> 8;
            }
            uint8_t tx_buff[32];
            uint16_t len = construct_msp_command_v1(tx_buff, MSP_DEBUG, payload_out, sizeof(payload_out), MSP_OUTBOUND);
            if (owner == MSP_OWNER_USB) usb_uart_write_bytes((const char *)tx_buff, len);
            else if (owner == MSP_OWNER_UART) uart1_tx_dma(tx_buff, len);
            break;
        }

        default:
            printf("MSP command not parsed %d:0x%02X\r\n",msp_cmd, msp_cmd);
            break;
        }
        break;
        case MSP_V2_OVER_V1:
            break;
        case MSP_V2_NATIVE:
            break;
        default:
            break;
    }

#if 0 // debug msp via usb-cdc
    for(int i = 0; i < data_size; i++) {
        printf("0x%02x ", payload[i]);
    }
    printf("\r\n");
#endif

    }
}

EXEC_RAM void msp_loop_process(void)
{
    uint8_t byte;
    while (uart_rx_ring_get(&byte)) {
        msp_process_received_data(&msp_uart, byte);
    }
    while (usb_uart_read_byte(&byte)) {
        msp_process_received_data(&msp_usb, byte);
    }

}
