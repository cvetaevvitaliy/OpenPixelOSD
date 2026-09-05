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
uint8_t boxIdIdx[3] = {0};

uint8_t mspStickpos(void) {
  uint8_t result = 0;
  for (uint8_t i = 0; i < 4; i++) {
    result >>= 2;
    if      (fc.rcChannel[i] > 500   && fc.rcChannel[i] < 1250  ) result |= 0x40;
    else if (fc.rcChannel[i] >= 1250 && fc.rcChannel[i] <= 1750 ) result |= 0x00;
    else if (fc.rcChannel[i] > 1750  && fc.rcChannel[i] < 2500  ) result |= 0x80;
    else result |= 0xc0;
  }
  return result;
}
__weak void cameraControl1_changed(bool value) {
  UNUSED(value);
}

__weak void cameraControl2_changed(bool value) {
  UNUSED(value);
}

__weak void cameraControl3_changed(bool value) {
  UNUSED(value);
}
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

        if (boxIdIdx[0] && (fc.status.cameraControl1 != ((status>>boxIdIdx[0]) & 0x01))) {
          fc.status.cameraControl1 = ((status>>boxIdIdx[0]) & 0x01);
          cameraControl1_changed(fc.status.cameraControl1);
          TRACE_INFO("CAMERA_CONTROL_1 %i\n", fc.status.cameraControl1);
        }   
        if (boxIdIdx[1] && (fc.status.cameraControl2 != ((status>>boxIdIdx[1]) & 0x01))) {
          fc.status.cameraControl2 = ((status>>boxIdIdx[1]) & 0x01);
          cameraControl2_changed(fc.status.cameraControl2);
          TRACE_INFO("CAMERA_CONTROL_2 %i\n", fc.status.cameraControl2);
        }
        if (boxIdIdx[2] && (fc.status.cameraControl3 != ((status>>boxIdIdx[2]) & 0x01))) {
          fc.status.cameraControl3 = ((status>>boxIdIdx[2]) & 0x01);
          cameraControl3_changed(fc.status.cameraControl3);
          TRACE_INFO("CAMERA_CONTROL_3 %i\n", fc.status.cameraControl3);
        }
        break;

    case MSP_BOXIDS:
        for (uint16_t i = 0; i < data_size; i++) {
          if (payload[i] == MSP_BOXID_CAMERA_CONTROL_1) {
            boxIdIdx[0] = i;
            TRACE_INFO("BOXID CAMERA_CONTROL_1 %02x\n", i)
          }
          if (payload[i] == MSP_BOXID_CAMERA_CONTROL_2) {
            boxIdIdx[1] = i;
            TRACE_INFO("BOXID CAMERA_CONTROL_2 %02x\n", i)
          }
          if (payload[i] == MSP_BOXID_CAMERA_CONTROL_3) {
            boxIdIdx[2] = i;
            TRACE_INFO("BOXID CAMERA_CONTROL_3 %02x\n", i)
          }
        }
        break;

    case MSP_RC:
        memcpy(fc.rcChannel, (uint16_t*)payload, sizeof(fc.rcChannel));
        fc.stickPos = mspStickpos();
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
