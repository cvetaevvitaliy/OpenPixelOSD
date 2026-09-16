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


/*
 * Betaflight permanent mode ID for CAMERA CONTROL 1.
 * MSP_BOXIDS tells us at which bit index this mode lives in MSP_STATUS.
 */
#define CAMERA_CONTROL_1_PERMANENT_ID       32U
#define CAMERA_MODE_INDEX_INVALID           0xFFU

#define MSP_BOXIDS_REQUEST_INTERVAL_MS      1000U
#define MSP_STATUS_REQUEST_INTERVAL_MS      100U

#define MSP_STATUS_MODE_FLAGS_OFFSET        6U
#define MSP_STATUS_BASE_MODE_BIT_COUNT      32U
#define MSP_STATUS_EXTRA_COUNT_OFFSET       15U
#define MSP_STATUS_EXTRA_FLAGS_OFFSET       16U


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
CCMRAM_DATA static uint8_t camera_control_1_mode_index =
    CAMERA_MODE_INDEX_INVALID;
CCMRAM_DATA static bool camera_control_1_state_valid = false;
CCMRAM_DATA static bool camera_control_1_last_state = false;

EXEC_RAM static void msp_callback(uint8_t owner, msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);


static void msp_uart_request(uint8_t command)
{
    uint8_t tx_buff[8];
    uint16_t len = construct_msp_command_v1(
        tx_buff,
        command,
        NULL,
        0,
        MSP_OUTBOUND
    );

    uart1_tx_dma(tx_buff, len);
}


static void camera_control_1_parse_box_ids(
    uint16_t data_size,
    const uint8_t *payload
)
{
    camera_control_1_mode_index = CAMERA_MODE_INDEX_INVALID;
    camera_control_1_state_valid = false;

    for (uint16_t i = 0; i < data_size; i++) {
        if (payload[i] == CAMERA_CONTROL_1_PERMANENT_ID) {
            camera_control_1_mode_index = (uint8_t)i;
            break;
        }
    }
}


static bool msp_status_get_mode_state(
    uint16_t data_size,
    const uint8_t *payload,
    uint8_t mode_index,
    bool *active
)
{
    uint16_t flag_offset;
    uint8_t bit_index;

    if (mode_index < MSP_STATUS_BASE_MODE_BIT_COUNT) {
        flag_offset = MSP_STATUS_MODE_FLAGS_OFFSET + (mode_index / 8U);
        bit_index = mode_index % 8U;
    } else {
        uint8_t extra_byte_count;
        uint8_t extended_index;

        if (data_size <= MSP_STATUS_EXTRA_COUNT_OFFSET) {
            return false;
        }

        extra_byte_count = payload[MSP_STATUS_EXTRA_COUNT_OFFSET];
        extended_index = mode_index - MSP_STATUS_BASE_MODE_BIT_COUNT;

        if ((extended_index / 8U) >= extra_byte_count) {
            return false;
        }

        flag_offset = MSP_STATUS_EXTRA_FLAGS_OFFSET + (extended_index / 8U);
        bit_index = extended_index % 8U;
    }

    if (flag_offset >= data_size) {
        return false;
    }

    *active = (payload[flag_offset] & (1U << bit_index)) != 0U;
    return true;
}


static void camera_control_1_parse_status(
    uint16_t data_size,
    const uint8_t *payload
)
{
    bool active;

    if (camera_control_1_mode_index == CAMERA_MODE_INDEX_INVALID) {
        return;
    }

    if (!msp_status_get_mode_state(
            data_size,
            payload,
            camera_control_1_mode_index,
            &active
        )) {
        return;
    }

    if (!camera_control_1_state_valid ||
        active != camera_control_1_last_state) {
        set_video_input(active ? VIDEO_INPUT_2 : VIDEO_INPUT_1);
        camera_control_1_last_state = active;
        camera_control_1_state_valid = true;
    }
}

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
        case MSP_BOXIDS:
            if (owner == MSP_OWNER_UART) {
                camera_control_1_parse_box_ids(data_size, payload);
            }
            break;

        case MSP_STATUS:
        case MSP_STATUS_EX:
            if (owner == MSP_OWNER_UART) {
                camera_control_1_parse_status(data_size, payload);
            }
            break;

        case MSP_DISPLAYPORT: {
            if (data_size < 1U) break;
            msp_displayport_cmd_t sub_cmd = payload[0];
            switch(sub_cmd) {
            case MSP_DISPLAYPORT_KEEPALIVE: // 0 -> Open/Keep-Alive DisplayPort
            {
                static bool displayport_initialized = false;
                if (!displayport_initialized) {
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


void msp_camera_switch_process(void)
{
    static uint32_t last_request_ms = 0;
    uint32_t now = HAL_GetTick();
    uint32_t request_interval;

    request_interval =
        (camera_control_1_mode_index == CAMERA_MODE_INDEX_INVALID)
            ? MSP_BOXIDS_REQUEST_INTERVAL_MS
            : MSP_STATUS_REQUEST_INTERVAL_MS;

    if ((uint32_t)(now - last_request_ms) < request_interval) {
        return;
    }

    last_request_ms = now;

    if (camera_control_1_mode_index == CAMERA_MODE_INDEX_INVALID) {
        msp_uart_request(MSP_BOXIDS);
    } else {
        msp_uart_request(MSP_STATUS);
    }
}
