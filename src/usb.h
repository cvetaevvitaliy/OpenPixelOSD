/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef USB_H
#define USB_H
#include <stdint.h>
#include <stdbool.h>

void usb_init(void);
bool usb_is_connected(void);
bool usb_uart_read_byte(uint8_t *byte);
uint16_t usb_uart_write_bytes(const char* data, uint16_t len);

#endif //USB_H
