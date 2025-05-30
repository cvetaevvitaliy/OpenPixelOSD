/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef MSP_H
#define MSP_H
#include <stdint.h>
#include <stdbool.h>
#include "msp_protocol.h"
#include "msp_protocol_v2_betaflight.h"
#include "msp_protocol_v2_common.h"

#define MSP_PORT_BUFF_SIZE    192
#define MSP_V2_FRAME_ID       255

typedef enum {
    MSP_IDLE,
    MSP_HEADER_START,
    MSP_HEADER_M,
    MSP_HEADER_X,

    MSP_HEADER_V1,
    MSP_PAYLOAD_V1,
    MSP_CHECKSUM_V1,

    MSP_HEADER_V2_OVER_V1,
    MSP_PAYLOAD_V2_OVER_V1,
    MSP_CHECKSUM_V2_OVER_V1,

    MSP_HEADER_V2_NATIVE,
    MSP_PAYLOAD_V2_NATIVE,
    MSP_CHECKSUM_V2_NATIVE,

    MSP_COMMAND_RECEIVED
}msp_state_t;

typedef enum {
    MSP_V1          = 0,
    MSP_V2_OVER_V1  = 1,
    MSP_V2_NATIVE   = 2,
    MSP_VERSION_COUNT
}msp_version_t;

typedef enum {
    MSP_PACKET_UNKNOWN,
    MSP_PACKET_COMMAND,
    MSP_PACKET_RESPONSE
}msp_packet_type_t;

typedef void (*msp_msg_callback)(uint8_t owner, msp_version_t msp_version, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);

typedef int msp_descriptor_t;

typedef struct mspPort_s {
    msp_state_t msp_state;
    uint8_t payload[MSP_PORT_BUFF_SIZE];
    uint16_t msp_cmd;
    uint8_t cmd_flags;
    msp_version_t msp_version;
    uint_fast16_t offset;
    uint_fast16_t data_size;
    uint8_t checksum1;
    uint8_t checksum2;
    msp_descriptor_t descriptor;
    msp_msg_callback callback;
    msp_packet_type_t packet_type;
    uint8_t owner;
}msp_port_t;

typedef struct __attribute__((packed)) {
    uint8_t size;
    uint8_t cmd;
}msp_header_v1_t;

typedef struct __attribute__((packed)) {
      uint8_t  flags;
      uint16_t cmd;
      uint16_t size;
}msp_header_v2_t;

typedef enum {
      MSP_ERR_NONE = 0,
      MSP_ERR_HDR,
      MSP_ERR_LEN,
      MSP_ERR_CKS,
      MSP_ERROR
}msp_error_t;

typedef enum {
      MSP_INBOUND,
      MSP_OUTBOUND
}msp_direction_t;

void msp_process_received_data(msp_port_t *mspPort, uint8_t c);

uint16_t construct_msp_command_v1(uint8_t message_buffer[], uint8_t command, const uint8_t *payload, uint8_t size, msp_direction_t direction);

uint16_t construct_msp_command_v2(uint8_t message_buffer[], uint16_t function, const uint8_t *payload, uint8_t size, msp_packet_type_t msp_packet_type);

#endif //MSP_H
