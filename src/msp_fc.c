/* SPDX-License-Identifier: GPL-2.0-only */
/**
 * Copyright (C) 2025 Vitaliy N <vitaliy.nimych@gmail.com>
 */
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "msp.h"

#include "msp_fc.h"

fc_t fc;

void msp_reboot(uint8_t rebootMode)
{
    if (rebootMode >= MSP_REBOOT_COUNT)
        return;

    LL_RTC_BKP_SetRegister(RTC, LL_RTC_BKP_DR1, rebootMode);

    __disable_irq();
    NVIC_SystemReset();
}

bool msp_fc_handle_msp(uint8_t owner, uint16_t msp_cmd, uint16_t data_size, const uint8_t *payload)
{
    uint32_t status;
    UNUSED(owner);

    switch(msp_cmd) {
    case MSP_STATUS:
        if (data_size < 10) {
            break; // malformed/short -- not enough payload to read the status word
        }
        memcpy(&status,&payload[6],4);

        if ( !fc.status.armed && (status & 0x01)) {
            TRACE_INFO("FC ARMED\n");
            fc.status.armed = 1;
        } else if ( fc.status.armed && !(status & 0x01)) {
            TRACE_INFO("FC DISARMED\n");
            fc.status.armed = 0;
        }
        break;

    case MSP_REBOOT:
        if(data_size > 0) {
          msp_reboot(payload[0]);
        }
        break;
    default:
        return false;
    }
    return true;
}
