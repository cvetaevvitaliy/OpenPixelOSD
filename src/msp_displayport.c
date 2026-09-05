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
#include "video_overlay.h"

#if defined(USE_VTX)
#include "vtx_msp.h"
#endif

typedef enum {
    MSP_DISPLAYPORT_KEEPALIVE,
    MSP_DISPLAYPORT_RELEASE,
    MSP_DISPLAYPORT_CLEAR,
    MSP_DISPLAYPORT_DRAW_STRING,
    MSP_DISPLAYPORT_DRAW_SCREEN,
    MSP_DISPLAYPORT_SET_OPTIONS,
    MSP_DISPLAYPORT_DRAW_SYSTEM,
    MSP_DISPLAYPORT_FONTCHAR_WRITE
} msp_displayport_cmd_t;

typedef enum {
    DISPLAYPORT_SYS_GOGGLE_VOLTAGE = 0,
    DISPLAYPORT_SYS_VTX_VOLTAGE = 1,
    DISPLAYPORT_SYS_BITRATE = 2,
    DISPLAYPORT_SYS_DELAY = 3,
    DISPLAYPORT_SYS_DISTANCE = 4,
    DISPLAYPORT_SYS_LQ = 5,
    DISPLAYPORT_SYS_GOGGLE_DVR = 6,
    DISPLAYPORT_SYS_VTX_DVR = 7,
    DISPLAYPORT_SYS_WARNINGS = 8,
    DISPLAYPORT_SYS_VTX_TEMP = 9,
    DISPLAYPORT_SYS_FAN_SPEED = 10,
    DISPLAYPORT_SYS_COUNT,
} displayPortSystemElement_e;

void msp_draw_system(uint8_t row, uint8_t col, uint8_t element) {
  char buffer[16];

   switch(element) {
    case DISPLAYPORT_SYS_VTX_VOLTAGE:
      {
        float vtxVoltage = adc1_read_mv(ADC1_CH_RESERVED);
        vtxVoltage = vtxVoltage / 500;
        snprintf(buffer, sizeof(buffer), "V %.1f%c", vtxVoltage, 0x06);
        canvas_char_write(col, row, (const char *)&buffer[0], 6, 0);
      }
      break;
    case DISPLAYPORT_SYS_VTX_TEMP:
      {
        float vtxTemp = adc_read_mcu_temp_c();
        snprintf(buffer, sizeof(buffer), "V%c %.0f%c", 0x7a, vtxTemp, 0x0e);
        canvas_char_write(col, row, (const char *)&buffer[0], 6, 0);
      }
      break;
/*    case DISPLAYPORT_SYS_LQ:
      {
        if(vtx_get_config()->pitmode) {
          snprintf(buffer, sizeof(buffer), "V%c%i   ", 0x15, vtx_get_power_mw());
        } else {
          snprintf(buffer, sizeof(buffer), "V %i   ", vtx_get_power_mw());
        }
        
        canvas_char_write(col, row, (const char *)&buffer[0], 5, 0);
      }
      break;*/
    default:
      break;
  }
    
}

EXEC_RAM bool msp_displayport_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{
    static bool displayport_initialized = false;

    switch(msp_cmd) {
    case MSP_DISPLAYPORT:
        if (osdState == OSD_MSP) {
            msp_displayport_cmd_t sub_cmd = payload[0];
            switch(sub_cmd) {
            case MSP_DISPLAYPORT_KEEPALIVE: // 0 -> Open/Keep-Alive DisplayPort
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
                break;

            case MSP_DISPLAYPORT_RELEASE: // 1 -> Close DisplayPort
                show_logo = true;
                break;

            case MSP_DISPLAYPORT_CLEAR: // 2 -> Clear Screen
                canvas_char_clean();
                break;

            case MSP_DISPLAYPORT_DRAW_STRING:  // 3 -> Draw String
                if (data_size < 5) break;
                uint8_t row = payload[1];
                uint8_t col = payload[2];
                uint8_t attribute = payload[3];

                if (row >= ROW_SIZE || col >= COLUMN_SIZE) break;
                uint8_t len = data_size - 4;
                canvas_char_write(col, row, (const char *)&payload[4], len, attribute & 0x03);
                break;

            case MSP_DISPLAYPORT_DRAW_SCREEN: // 4 -> Draw Screen
                canvas_char_draw_complete();
                break;

            case MSP_DISPLAYPORT_SET_OPTIONS: // 5 -> Set Options (HDZero/iNav)
                break;
            
            case MSP_DISPLAYPORT_DRAW_SYSTEM:
                msp_draw_system(payload[1], payload[2], payload[3]);
                break;

            case MSP_DISPLAYPORT_FONTCHAR_WRITE:
                #ifndef USE_COLOR
                update_font_symbol_write_bulk(payload[1], &payload[4], data_size - 4);
                #endif
                break;

            default:
                break;
            }
        }

        break;

    case  MSP_OSD_CHAR_WRITE:
        update_font_symbol_write(payload[0], &payload[1], data_size - 1);
        break;
    default:
        return false;
        break;
    }
   
    return true;
}


