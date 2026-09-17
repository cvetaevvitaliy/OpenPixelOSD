/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "msp.h"
#include "msp_fc.h"
#include "msp_displayport.h"
#include "uart.h"
#include "usb.h"

#if defined(USE_VTX)
#include "vtx_msp.h"
#endif

#define MSP_REQUEST_LOOP_INTERVAL 100u

CCMRAM_BSS static msp_port_t msp_uart = {0};
CCMRAM_BSS static msp_port_t msp_usb = {0};
EXEC_RAM static void msp_callback(uint8_t owner, msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);


void msp_init(void)
{
    uart1_init();
    uart1_dma_rx_start();
    msp_uart.callback = msp_callback;
    msp_uart.owner = MSP_OWNER_UART;

    msp_usb.callback = msp_callback;
    msp_usb.owner = MSP_OWNER_USB;
    
    msp_send_command(MSP_OWNER_UART, MSP_BOXIDS);
}

EXEC_RAM static void msp_callback(uint8_t owner, msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{
    UNUSED(msp_version);

    switch(msp_cmd) {
    case MSP_DISPLAYPORT: 
    case MSP_OSD_CHAR_WRITE:
        msp_displayport_handle_msp(owner, msp_cmd, data_size, payload);
        break;

    case MSP_VTX_CONFIG:
    case MSP_SET_VTX_CONFIG:
    case MSP_VTXTABLE_BAND:
    case MSP_VTXTABLE_POWERLEVEL:
    case MSP_PACALTABLE:
    case MSP_SET_PACALTABLE:
    case MSP_PACALIBRATION:
    case MSP_SET_PACALIBRATION:
    case MSP_EEPROM_WRITE:
#if defined(USE_VTX)
        vtx_msp_handle_msp(owner, msp_cmd, data_size, payload);
#endif
        break;
    case MSP_STATUS:
    case MSP_BOXIDS:
    case MSP_RC:
    case MSP_REBOOT:
    case MSP_DEBUG:
        msp_fc_handle_msp(owner, msp_cmd, data_size, payload);
        break;
    default:
        printf("MSP command not parsed %d:0x%02X\r\n",msp_cmd, msp_cmd);
        break;
    }
}


EXEC_RAM static uint8_t crc8_calc(uint8_t crc, unsigned char a, uint8_t poly)
{
    crc ^= a;
    for (int ii = 0; ii < 8; ++ii) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ poly;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}


EXEC_RAM void msp_process_received_data(msp_port_t *mspPort, uint8_t c)
{
    switch (mspPort->msp_state) {
    default:
    case MSP_IDLE:      // Waiting for '$' character
        if (c == '$') {
            mspPort->msp_state = MSP_HEADER_START;
        } else {
            return;
        }
        break;

    case MSP_HEADER_START:  // Waiting for 'M' (MSPv1 / MSPv2_over_v1) or 'X' (MSPv2 native)
        mspPort->offset = 0;
        mspPort->checksum1 = 0;
        mspPort->checksum2 = 0;
        switch (c) {
        case 'M':
            mspPort->msp_state = MSP_HEADER_M;
            mspPort->msp_version = MSP_V1;
            break;
        case 'X':
            mspPort->msp_state = MSP_HEADER_X;
            mspPort->msp_version = MSP_V2_NATIVE;
            break;
        default:
            mspPort->msp_state = MSP_IDLE;
            break;
        }
        break;

    case MSP_HEADER_M:      // Waiting for '<' / '>'
        mspPort->msp_state = MSP_HEADER_V1;
        switch (c) {
        case '<':
            mspPort->packet_type = MSP_PACKET_COMMAND;
            break;
        case '>':
            mspPort->packet_type = MSP_PACKET_RESPONSE;
            break;
        default:
            mspPort->packet_type = MSP_PACKET_UNKNOWN;
            mspPort->msp_state = MSP_IDLE;
            break;
        }
        break;

    case MSP_HEADER_X:
        mspPort->msp_state = MSP_HEADER_V2_NATIVE;
        switch (c) {
        case '<':
            mspPort->packet_type = MSP_PACKET_COMMAND;
            break;
        case '>':
            mspPort->packet_type = MSP_PACKET_RESPONSE;
            break;
        default:
            mspPort->packet_type = MSP_PACKET_UNKNOWN;
            mspPort->msp_state = MSP_IDLE;
            break;
        }
        break;

    case MSP_HEADER_V1:     // Now receive v1 header (size/cmd), this is already checksummable
        mspPort->payload[mspPort->offset++] = c;
        mspPort->checksum1 ^= c;
        if (mspPort->offset == sizeof(msp_header_v1_t)) {
            msp_header_v1_t * hdr = (msp_header_v1_t *)&mspPort->payload[0];
            // Check incoming buffer size limit
            if (hdr->size > MSP_PORT_BUFF_SIZE) {
                mspPort->msp_state = MSP_IDLE;
            }
            else if (hdr->cmd == MSP_V2_FRAME_ID) {
                // MSPv1 payload must be big enough to hold V2 header + extra checksum
                if (hdr->size >= sizeof(msp_header_v2_t) + 1) {
                    mspPort->msp_version = MSP_V2_OVER_V1;
                    mspPort->msp_state = MSP_HEADER_V2_OVER_V1;
                } else {
                    mspPort->msp_state = MSP_IDLE;
                }
            } else {
                mspPort->data_size = hdr->size;
                mspPort->msp_cmd = hdr->cmd;
                mspPort->cmd_flags = 0;
                mspPort->offset = 0;                // re-use buffer
                mspPort->msp_state = mspPort->data_size > 0 ? MSP_PAYLOAD_V1 : MSP_CHECKSUM_V1;    // If no payload - jump to checksum byte
            }
        }
        break;

    case MSP_PAYLOAD_V1:
        mspPort->payload[mspPort->offset++] = c;
        mspPort->checksum1 ^= c;
        if (mspPort->offset == mspPort->data_size) {
            mspPort->msp_state = MSP_CHECKSUM_V1;
        }
        break;

    case MSP_CHECKSUM_V1:
        if (mspPort->checksum1 == c) {
            mspPort->msp_state = MSP_COMMAND_RECEIVED;
            if (mspPort->callback != 0) {
                mspPort->callback(mspPort->owner, mspPort->msp_version, mspPort->msp_cmd, mspPort->data_size, mspPort->payload);
                mspPort->msp_state = MSP_IDLE;
            }
        } else {
            mspPort->msp_state = MSP_IDLE;
        }
        break;

    case MSP_HEADER_V2_OVER_V1:     // V2 header is part of V1 payload - we need to calculate both checksums now
        mspPort->payload[mspPort->offset++] = c;
        mspPort->checksum1 ^= c;
        mspPort->checksum2 = crc8_calc(mspPort->checksum2, c, 0xD5);
        if (mspPort->offset == (sizeof(msp_header_v2_t) + sizeof(msp_header_v1_t))) {
            msp_header_v2_t * hdrv2 = (msp_header_v2_t *)&mspPort->payload[sizeof(msp_header_v1_t)];
            if (hdrv2->size > MSP_PORT_BUFF_SIZE) {
                mspPort->msp_state = MSP_IDLE;
            } else {
                mspPort->data_size = hdrv2->size;
                mspPort->msp_cmd = hdrv2->cmd;
                mspPort->cmd_flags = hdrv2->flags;
                mspPort->offset = 0;                // re-use buffer
                mspPort->msp_state = mspPort->data_size > 0 ? MSP_PAYLOAD_V2_OVER_V1 : MSP_CHECKSUM_V2_OVER_V1;
            }
        }
        break;

    case MSP_PAYLOAD_V2_OVER_V1:
        mspPort->checksum2 = crc8_calc(mspPort->checksum2, c, 0xD5);
        mspPort->checksum1 ^= c;
        mspPort->payload[mspPort->offset++] = c;

        if (mspPort->offset == mspPort->data_size) {
            mspPort->msp_state = MSP_CHECKSUM_V2_OVER_V1;
        }
        break;

    case MSP_CHECKSUM_V2_OVER_V1:
        mspPort->checksum1 ^= c;
        if (mspPort->checksum2 == c) {
            mspPort->msp_state = MSP_CHECKSUM_V1; // Checksum 2 correct - verify v1 checksum
        } else {
            mspPort->msp_state = MSP_IDLE;
        }
        break;

    case MSP_HEADER_V2_NATIVE:
        mspPort->payload[mspPort->offset++] = c;
        mspPort->checksum2 = crc8_calc(mspPort->checksum2, c, 0xD5);
        if (mspPort->offset == sizeof(msp_header_v2_t)) {
            msp_header_v2_t * hdrv2 = (msp_header_v2_t *)&mspPort->payload[0];
            // Bounds check to prevent memory corruption due to invalid or garbled
            // data transmission.
            if (hdrv2->size > MSP_PORT_BUFF_SIZE) {
                mspPort->msp_state = MSP_IDLE;
            } else {
                mspPort->data_size = hdrv2->size;
                mspPort->msp_cmd = hdrv2->cmd;
                mspPort->cmd_flags = hdrv2->flags;
                mspPort->offset = 0;                // re-use buffer
                mspPort->msp_state = mspPort->data_size > 0 ? MSP_PAYLOAD_V2_NATIVE : MSP_CHECKSUM_V2_NATIVE;
            }
        }
        break;

    case MSP_PAYLOAD_V2_NATIVE:
        mspPort->checksum2 = crc8_calc(mspPort->checksum2, c, 0xD5);
        mspPort->payload[mspPort->offset++] = c;

        if (mspPort->offset == mspPort->data_size) {
            mspPort->msp_state = MSP_CHECKSUM_V2_NATIVE;
        }
        break;

    case MSP_CHECKSUM_V2_NATIVE:
        if (mspPort->checksum2 == c) {
            mspPort->msp_state = MSP_COMMAND_RECEIVED;
            if (mspPort->callback != 0) {
                mspPort->callback(mspPort->owner, mspPort->msp_version, mspPort->msp_cmd, mspPort->data_size, mspPort->payload);
                mspPort->msp_state = MSP_IDLE;
            }
        } else {
            mspPort->msp_state = MSP_IDLE;
        }
        break;
    }
}

uint16_t construct_msp_command_v1(uint8_t message_buffer[], uint8_t command, const uint8_t *payload, uint8_t size, msp_direction_t direction)
{
    uint8_t checksum;
    message_buffer[0] = '$'; // Header
    message_buffer[1] = 'M'; // MSP V1
    if (direction == MSP_OUTBOUND) {
        message_buffer[2] = '<';
    } else {
        message_buffer[2] = '>';
    }
    message_buffer[3] = size; // Payload Size
    checksum = size;
    message_buffer[4] = command; // Command
    checksum ^= command;
    for(uint8_t i = 0; i < size; i++) {
        message_buffer[5 + i] = payload[i];
        checksum ^= message_buffer[5 + i];
    }
    message_buffer[5 + size] = checksum;
    return 6 + size;
}

uint16_t construct_msp_command_v2(uint8_t message_buffer[], uint16_t function, const uint8_t *payload, uint8_t size, msp_packet_type_t msp_packet_type)
{
    uint8_t checksum = 0;
    uint8_t len = 0;

    message_buffer[0] = '$'; // Header
    message_buffer[1] = 'X'; // MSP V2
    if (msp_packet_type == MSP_PACKET_COMMAND) {
        message_buffer[2] = '<';
    } else if (msp_packet_type == MSP_PACKET_RESPONSE){
        message_buffer[2] = '>';
    } else {
        return 0;
    }

    // Pack header struct into message_buffer
    msp_header_v2_t header;
    header.flags = 0;
    header.cmd = function;
    header.size = size;

    // Copy header buffer, adding each byte to the crc
    memcpy(message_buffer+3, &header, sizeof(msp_header_v2_t));
    for (int i = 0; i < 5; i++) {
        checksum = crc8_calc(checksum, message_buffer[3 + i], 0xD5);
    }

    // Copy payload, adding each byte to the crc
    if (size > 0 && payload != NULL) {
        memcpy(message_buffer+8, payload, size);
        for (int i = 0; i < size; i++) {
            checksum = crc8_calc(checksum, message_buffer[8 + i], 0xD5);
        }
    }

    // Add checksum to message buffer
    message_buffer[8 + size] = checksum;
    len = 3 + 5 + size + 1;

#if 0 // debug construct packet
    printf("msp_buffer: ");
    for (int i = 0; i < len; ++i) {
        printf("0x%02X ", message_buffer[i]);
    }
    printf("crc: %02X\n", checksum);
#endif
    return len;
}

void msp_tx_send_owner(uint8_t owner, const uint8_t *buf, uint16_t len)
{
    if (owner == MSP_OWNER_USB) {
        usb_uart_write_bytes((const char*)buf, len);
    } else if (owner == MSP_OWNER_UART) {
        uart1_tx_dma((uint8_t*)buf, len);
    }
}

void msp_send_command(uint8_t owner, uint8_t command)
{
    uint8_t tx_buff[64];
    const uint16_t len = construct_msp_command_v1(tx_buff, command, NULL, 0, MSP_OUTBOUND);
    msp_tx_send_owner(owner, tx_buff, len);
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

    static uint32_t last_tick = 0;
    static uint8_t configRequest = 2;
    static uint8_t c = 0;
    if ((HAL_GetTick() - last_tick) >= (MSP_REQUEST_LOOP_INTERVAL)) {
        last_tick = HAL_GetTick();
        switch(c) {
          case 0:
          case 2:
            msp_send_command(MSP_OWNER_UART, MSP_RC);
            c++;
            break;
          case 1:
            msp_send_command(MSP_OWNER_UART, MSP_STATUS);
            c = c + (!fc.status.armed);
            break;
          case 3:
#if defined(USE_VTX)
            if (!vtx_get_config()->configSet) {
              if(configRequest) {
                vtx_msp_request_config(MSP_OWNER_UART);
              } else {
                vtx_set_pitmode(0);
                vtx_config_t *vtx_config = (vtx_config_t*)vtx_get_config();
                vtx_config->configSet = 1;
              }
            }
#else
            UNUSED(configRequest);
#endif 
            c = 0;
            break;
        }
    }
}