/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include "main.h"

void OPAMP1_Init(void)
{
    LL_OPAMP_InitTypeDef OPAMP_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    /**OPAMP1 GPIO Configuration
    PA1   ------> OPAMP1_VINP_SEC - NOT useed now
    PA2   ------> OPAMP1_VOUT - Video output
    PA3   ------> OPAMP1_VINP IO1 - Camera 1 video input
    PA7   ------> OPAMP1_VINP IO2 - Camera 2 video input
    */
    GPIO_InitStruct.Pin = OPAMP1_VINPIO0_GRAY_COLOR_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(OPAMP1_VINPIO0_GRAY_COLOR_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = OPAMP1_VOUT_VIDEO_OUT_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(OPAMP1_VOUT_VIDEO_OUT_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = OPAMP1_VINPIO1_VIDEO1_IN_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(OPAMP1_VINPIO1_VIDEO1_IN_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = OPAMP1_VINPIO2_VIDEO2_IN_Pin;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(OPAMP1_VINPIO2_VIDEO2_IN_GPIO_Port, &GPIO_InitStruct);

    OPAMP_InitStruct.PowerMode = LL_OPAMP_POWERMODE_HIGHSPEED;
    OPAMP_InitStruct.FunctionalMode = LL_OPAMP_MODE_FOLLOWER;
    OPAMP_InitStruct.InputNonInverting = LL_OPAMP_INPUT_NONINVERT_IO1;
    LL_OPAMP_Init(OPAMP1, &OPAMP_InitStruct);
    LL_OPAMP_SetInputsMuxMode(OPAMP1, LL_OPAMP_INPUT_MUX_DISABLE);
    LL_OPAMP_SetInternalOutput(OPAMP1, LL_OPAMP_INTERNAL_OUPUT_DISABLED);
    LL_OPAMP_SetTrimmingMode(OPAMP1, LL_OPAMP_TRIMMING_FACTORY);
}
