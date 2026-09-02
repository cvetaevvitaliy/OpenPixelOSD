/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "msp_displayport.h"
#include "canvas_char.h"
#include "fonts/update_font.h"
#include "main.h"
#include "msp.h"
#include "uart.h"
#include "usb.h"

#if defined(USE_VTX)
#include "vtx_msp.h"
#define MSP_REQUEST_LOOP_INTERVAL 1000
#endif

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


EXEC_RAM bool msp_displayport_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{

    switch(msp_cmd) {
        case MSP_DISPLAYPORT: {
            msp_displayport_cmd_t sub_cmd = payload[0];
            switch(sub_cmd) {
            case MSP_DISPLAYPORT_KEEPALIVE: // 0 -> Open/Keep-Alive DisplayPort
            {
                static bool displayport_initialized = false;
                if (!displayport_initialized) {
                    #if defined(USE_VTX)
                    vtx_msp_request_config(owner);
                    #endif
                    displayport_initialized = true;
                    show_logo = false;
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
                uint8_t len = data_size - 4;

                uint8_t max_len = COLUMN_SIZE - col;
                if (len > max_len) {
                    len = max_len;
                }

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
        break;

        case  MSP_OSD_CHAR_WRITE: {
            update_font_symbol_write(payload[0], &payload[1], data_size - 1);
        }
        break;

        default:
            return false;
    }
    return true;
}

