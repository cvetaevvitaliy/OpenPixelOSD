#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "video_color.h"
#include "video_overlay.h"
#include "main.h"

uint8_t colorMapIdx = 0;

const colorMap_t colorMap[][16] =  { 
                                    { {0.0f,    0},       // black
                                      {-1.0f,   100},     // transparent
                                      {0.0f,    700},     // white
                                      {0.0f,    245},     // grey 35%
                                      {240.7f,  440},     // green
                                      {103.5f,  300},     // bright red
                                      {347.1f,  150},     // bright blue
                                      {167.1f,  660},     // yellow

                                      {139.3f,  470},     // amber
                                      {283.5f,  520},     // cyan
                                      { 60.7f,  300},     // magenta
                                      {103.5f,  220},     // red
                                      {347.1f,  100},     // blue
                                      {0.0f,    175},     // grey 25% 
                                      {0.0f,    350},     // grey 50% 
                                      {0.0f,    525}      // grey 75% 
                                    }
                                  };


#ifdef USE_COLOR

uint32_t phase_buff[1 + DMA_DOUBLE_BUFFER][LINE_BUF_SZ + 8]; // double buffer for color phase  DMA WORLD/WORLD
uint32_t phase_val[2][16] = {0};

uint8_t palPhase = 0;
uint16_t colorDelay = 0;
float phaseOffset = 0;


void video_color_init(void)
{
    TIM3_Init();
    HRTIM1_Init();

    LL_TIM_EnableIT_CC1(TIM3);
    LL_TIM_EnableIT_UPDATE(TIM3);
    LL_TIM_EnableCounter(TIM3);
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);
}

void set_color_system(videoMode_t mode)
{
  if (mode == MODE_NTSC) {
    LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_A, HRTIM_RELOAD_NTSC);
    LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_B, HRTIM_RELOAD_NTSC);
    LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_C, HRTIM_RELOAD_NTSC);
    colorDelay = COLOR_DELAY_NTSC;
  } else {
    LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_A, HRTIM_RELOAD_PAL);
    LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_B, HRTIM_RELOAD_PAL);
    LL_HRTIM_TIM_SetPeriod(HRTIM1, LL_HRTIM_TIMER_C, HRTIM_RELOAD_PAL);
    colorDelay = COLOR_DELAY_PAL;
  }

  LL_TIM_OC_SetCompareCH1(TIM1, TIM1_AUTORELOAD - (colorDelay % TIM1_AUTORELOAD));
}

void set_color_phase(videoMode_t mode)
{
  if (mode == MODE_NTSC) {
    for (uint8_t x = 0; x<16; x++) {
      if (colorMap[colorMapIdx][x].phase > 0) {
        phase_val[0][x] = MAX(32,(uint16_t)((phaseOffset + 625.0f - colorMap[colorMapIdx][x].phase ) / 360 * HRTIM_RELOAD_NTSC) % HRTIM_RELOAD_NTSC);
        phase_val[1][x] = MAX(32,(uint16_t)((phaseOffset + 625.0f - colorMap[colorMapIdx][x].phase ) / 360 * HRTIM_RELOAD_NTSC) % HRTIM_RELOAD_NTSC);
      } else if (colorMap[colorMapIdx][x].phase == 0) {
        phase_val[0][x] = 0;
        phase_val[1][x] = 0;
      } else {
        phase_val[0][x] = 0;
        phase_val[1][x] = 0;
      }
    }

  } else {
    for (uint8_t x = 0; x<16; x++) {
      if (colorMap[colorMapIdx][x].phase > 0) {
        phase_val[0][x] = MAX(32,(uint16_t)((phaseOffset + 640.0f - colorMap[colorMapIdx][x].phase - 90.0f) / 360 * HRTIM_RELOAD_PAL) % HRTIM_RELOAD_PAL);
        phase_val[1][x] = MAX(32,(uint16_t)((phaseOffset + 640.0f + colorMap[colorMapIdx][x].phase        ) / 360 * HRTIM_RELOAD_PAL) % HRTIM_RELOAD_PAL);
      } else if (colorMap[colorMapIdx][x].phase == 0) {
        phase_val[0][x] = 0;
        phase_val[1][x] = 0;
      } else {
        phase_val[0][x] = 0;
        phase_val[1][x] = 0;
      }
    }
  }
}


// Called from COMP2 & TIM3 event (video input)
EXEC_RAM void TIM3_IRQHandler(void)
{  
  static uint8_t phase = 0;

  if (LL_TIM_IsActiveFlag_UPDATE(TIM3)) {
    palPhase = phase;
    // Stop synchronizing HRTIM TIMER C with TIMER B
    LL_HRTIM_TIM_SetResetTrig(HRTIM1, LL_HRTIM_TIMER_C, LL_HRTIM_RESETTRIG_NONE);
    LL_TIM_ClearFlag_UPDATE(TIM3);
  }

  if (LL_TIM_IsActiveFlag_CC1(TIM3)) {
    if(LL_HRTIM_TIM_GetResetTrig(HRTIM1, LL_HRTIM_TIMER_B) == LL_HRTIM_RESETTRIG_EEV_1) {
      // Close trigger gate for HRTIM TIMER B colorbust synchronisation
      LL_HRTIM_TIM_SetResetTrig(HRTIM1, LL_HRTIM_TIMER_B, LL_HRTIM_RESETTRIG_NONE);

      //Determine PAL phase
      if(LL_HRTIM_TIM_GetCapture1(HRTIM1, LL_HRTIM_TIMER_B) > (HRTIM_RELOAD_PAL / 2) ) {
        phase = 1 ^ DMA_DOUBLE_BUFFER;
      } else {
        phase = 0 ^ DMA_DOUBLE_BUFFER;
      }

      // Start synchronizing HRTIM TIMER C with TIMER B
      LL_HRTIM_TIM_SetResetTrig(HRTIM1, LL_HRTIM_TIMER_C, LL_HRTIM_RESETTRIG_OTHER2_CMP2); 
    }

    LL_TIM_ClearFlag_CC1(TIM3);
  }
  
}


#endif