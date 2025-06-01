/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <string.h>
#include <stdbool.h>
#include "usb.h"
#include "main.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"


#define RX_BUFFER_SIZE 512 // can be reduced to save RAM
#define TX_BUFFER_SIZE 512
#define FIRST_TX_DELAY_MS 150U // Delay before the first transmission after USB connection, tim7 tick - 1MHz (10ms)

static const char tx_overflow_msg[] = "\r\n*TxBuffFull*\r\n";

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;

static uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;

static volatile uint8_t tx_busy = 0;

extern USBD_HandleTypeDef hUsbDeviceFS;

static void usb_uart_start_transmit(void);

#define OPENPIXELOSD_BANNER "**** OpenPixelOSD© ****\r\nFW:%s %s-%s\r\nMCU:%s\r\n", FW_VERSION, GIT_BRANCH, GIT_HASH, MCU_TYPE

static void print_banner(void)
{
    printf("\033[2J\033[H"); // Clear console
    printf(OPENPIXELOSD_BANNER);
    printf("Using USB may introduce artifacts into the analog signal.\r\n");
}

void usb_init(void)
{
    MX_USB_Device_Init();
    LL_mDelay(500);
    TIM7_Init();
    LL_TIM_EnableCounter(TIM7);
    LL_TIM_EnableIT_UPDATE(TIM7);
}

bool usb_is_connected(void)
{
    return (hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED);
}

uint16_t usb_uart_write_bytes(const char* data, uint16_t len)
{
    uint16_t bytes_written = 0;

    // Calculate free space in the buffer
    uint16_t free_space;
    if (tx_head >= tx_tail)
        free_space = TX_BUFFER_SIZE - (tx_head - tx_tail) - 1;
    else
        free_space = (tx_tail - tx_head) - 1;

    size_t msg_len = strlen(tx_overflow_msg);

    // If there is not enough space for new data + message, add only the message
    if (free_space < len + msg_len) {
        if (free_space >= msg_len) {
            // Writing an overflow message to the circular buffer
            uint16_t first_chunk = TX_BUFFER_SIZE - tx_head;
            if (first_chunk > msg_len)
                first_chunk = msg_len;

            memcpy(&tx_buffer[tx_head], tx_overflow_msg, first_chunk);
            tx_head = (tx_head + first_chunk) % TX_BUFFER_SIZE;

            uint16_t second_chunk = msg_len - first_chunk;
            if (second_chunk > 0) {
                memcpy(&tx_buffer[tx_head], tx_overflow_msg + first_chunk, second_chunk);
                tx_head = (tx_head + second_chunk) % TX_BUFFER_SIZE;
            }
        }
        return 0; // New data is not written
    }

    // If there is enough space, write new data to the circular buffer
    // First block (up to the end of the buffer)
    uint16_t first_chunk = TX_BUFFER_SIZE - tx_head;
    if (first_chunk > len)
        first_chunk = len;

    memcpy(&tx_buffer[tx_head], data, first_chunk);
    tx_head = (tx_head + first_chunk) % TX_BUFFER_SIZE;
    bytes_written += first_chunk;

    // Second block (from the beginning of the buffer)
    uint16_t second_chunk = len - first_chunk;
    if (second_chunk > 0) {
        memcpy(&tx_buffer[tx_head], data + first_chunk, second_chunk);
        tx_head = (tx_head + second_chunk) % TX_BUFFER_SIZE;
        bytes_written += second_chunk;
    }

    return bytes_written;
}

static void usb_uart_start_transmit(void)
{
    if (tx_head == tx_tail) {
        tx_busy = 0;
        return;// No data for transmission
    }

    tx_busy = 1;

    uint16_t size_to_send;
    if (tx_head > tx_tail) {
        size_to_send = tx_head - tx_tail;
    }
    else {
        size_to_send = TX_BUFFER_SIZE - tx_tail;
    }

    uint8_t res = CDC_Transmit_FS(&tx_buffer[tx_tail], size_to_send);
    if (res != USBD_OK) {
        tx_busy = 0;
    }
}

int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
    UNUSED(Buf);
    UNUSED(epnum);
    tx_tail = (tx_tail + *Len) % TX_BUFFER_SIZE;

    if (tx_tail != tx_head) {
        usb_uart_start_transmit();
    }
    else {
        tx_busy = 0;
    }

    return USBD_OK;
}

int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
    uint32_t len = *Len;

    for (uint32_t i = 0; i < len; i++) {
        uint16_t next_head = (rx_head + 1) % RX_BUFFER_SIZE;
        if (next_head != rx_tail) {
            rx_buffer[rx_head] = Buf[i];
            rx_head = next_head;
        } else {
            break;
        }
    }

    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);

    return USBD_OK;
}

bool usb_uart_read_byte(uint8_t *byte)
{
    if (rx_head == rx_tail)
        return false;

    *byte = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return true;
}

void TIM7_IRQHandler(void)
{
    static volatile uint32_t tim7_ticks = 0;
    static volatile bool usb_connected_prev = false;
    static volatile bool first_tx_done = false;

    if (LL_TIM_IsActiveFlag_UPDATE(TIM7)) {
        LL_TIM_ClearFlag_UPDATE(TIM7);

        bool usb_connected = usb_is_connected();

        if (usb_connected) {
            if (!usb_connected_prev) {
                // USB just connected, starting delay countdown
                tim7_ticks = 0;
                first_tx_done = false;
                print_banner();
            }

            if (!first_tx_done) {
                tim7_ticks++;
                if (tim7_ticks >= FIRST_TX_DELAY_MS) {
                    first_tx_done = true; // Delay is over, transmission can start
                }
            } else {
               // Delay passed, transmission can start
                if (!tx_busy && (tx_tail != tx_head)) {
                    usb_uart_start_transmit();
                } else {
                    tx_busy = 0;
                }
            }
        } else {
           // USB disconnected — resetting state
            usb_connected_prev = false;
            first_tx_done = false;
            tim7_ticks = 0;
            tx_busy = 0;
            return;
        }

        usb_connected_prev = usb_connected;
    }
}
