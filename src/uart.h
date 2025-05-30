/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#ifndef UART_H
#define UART_H
#include <stdint.h>
#include <stdbool.h>

void uart1_init(void);
void uart1_dma_rx_start(void);
bool uart_rx_ring_get(uint8_t *data);
void uart1_tx_dma(uint8_t *data, uint32_t len);

#endif //UART_H
