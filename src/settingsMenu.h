#pragma once

#include <stdint.h>

typedef enum {
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_MID,
    BTN_ENTER,
    BTN_EXIT,
    BTN_ENTER_VTX,
    BTN_INVALID
} ButtonEvent_e;

typedef void (*osdPrintFuncPtr)(uint8_t x, uint8_t y, uint8_t idx);
typedef void (*osdKeyFuncPtr)(ButtonEvent_e btn, uint8_t idx);

typedef struct {
    uint8_t idx;
    const char *text;
    osdPrintFuncPtr printFunc;
    osdKeyFuncPtr keyFunc;
} osdEntry_t;

void msp_menu(void);