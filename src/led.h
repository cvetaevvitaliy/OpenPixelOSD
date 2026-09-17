#pragma once

#include <stdint.h>

#define RGB_LED_COUNT         5

#define RGB(R,G,B)            ((B)+((R)<<8)+((G)<<16))

#define LED_STATE             64
#define TP1                   65
#define TP2                   66

#define RGB_YELLOW          RGB(64,64, 0)
#define RGB_GREEN           RGB( 0,64, 0)
#define RGB_RED             RGB(64, 0, 0)
#define RGB_BLUE            RGB( 0, 0,64)
#define RGB_AMBER           RGB(64,48, 0)
#define RGB_WHITE           RGB(64,64,64)

void led_init(void);
void led_set(uint8_t idx, uint32_t value);
void led_toggle(uint8_t idx);
void RGB_led_send(void);