#pragma once

#include "main.h"
#include "video_overlay.h"

#define HRTIM_RELOAD_PAL    1227
#define HRTIM_RELOAD_NTSC   1520

extern const colorMap_t colorMap[][16];
extern uint32_t phase_buff[1 + DMA_DOUBLE_BUFFER][LINE_BUF_SZ + 8];
extern uint32_t phase_val[2][16];
extern uint8_t palPhase;
extern uint8_t colorMapIdx;
extern uint16_t colorDelay;

void video_color_init(void);
void set_color_system(videoMode_t mode);
void set_color_phase(videoMode_t mode);
