/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef MSP_DISPLAYPORT_H
#define MSP_DISPLAYPORT_H
#include <stdint.h>

bool msp_displayport_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload);

#endif //MSP_DISPLAYPORT_H
