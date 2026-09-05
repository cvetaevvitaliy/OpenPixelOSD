#include <string.h>
#include "main.h"

#include "settingsMenu.h"
#include "settings.h"
#if defined(USE_VTX)
#include "vtx_msp.h"
#include "rf_pa.h"
#endif
#include "canvas_char.h"
#include "video_overlay.h"
#include "msp_displayport.h"
#include "msp_fc.h"

#define OSD_MENU_TOP                2
#define OSD_MENU_TEXT_LEFT          ((COLUMN_SIZE - 24) / 2)
#define OSD_MENU_VALUE_LEFT         (OSD_MENU_TEXT_LEFT + 14 )

uint8_t tempChannel;
uint8_t tempBand;
uint8_t tempVideoInput;

void printMenuValueVtx(uint8_t x, uint8_t y, uint8_t idx);
void printMenuValue(uint8_t x, uint8_t y, uint8_t idx);
void changeChannel(ButtonEvent_e btn, uint8_t idx);
void changePower(ButtonEvent_e btn, uint8_t idx);
void exitVtxMenu(ButtonEvent_e btn, uint8_t idx);
void exitVtxMenu(ButtonEvent_e btn, uint8_t idx);
void changePit(ButtonEvent_e btn, uint8_t idx);
void changeDisplayport(ButtonEvent_e btn, uint8_t idx);
void changeVideoIn(ButtonEvent_e btn, uint8_t idx);

typedef enum {
  MENU_EXIT = 0,
  MENU_SAVE_EXIT,
  MENU_BAND,
  MENU_CHANNEL,
  MENU_FREQUENCY,
  MENU_POWER,
  MENU_PIT_MODE,
  MENU_DISPLAYPORT,
  MENU_VIDEO_INPUT
} menuIdx_t;

osdEntry_t osdMenue[] = { 
                          #if defined(USE_VTX)
                          {MENU_BAND,         "BAND",        (osdPrintFuncPtr)printMenuValueVtx, (osdKeyFuncPtr)changeChannel},
                          {MENU_CHANNEL,      "CHANNEL",     (osdPrintFuncPtr)printMenuValueVtx, (osdKeyFuncPtr)changeChannel},
                          {MENU_FREQUENCY,    "FREQUENCY",   (osdPrintFuncPtr)printMenuValueVtx, NULL},
                          {MENU_POWER,        "POWER",       (osdPrintFuncPtr)printMenuValueVtx, (osdKeyFuncPtr)changePower},
                          {MENU_PIT_MODE,     "PIT MODE",    (osdPrintFuncPtr)printMenuValueVtx, (osdKeyFuncPtr)changePit},
                          #endif
                          {MENU_DISPLAYPORT,  "DISPLAYPORT", (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeDisplayport},
                          #if (VIDEO1_INPUT_ENABLED == true && VIDEO2_INPUT_ENABLED == true)
                          {MENU_VIDEO_INPUT,  "VIDEO INPUT", (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeVideoIn},
                          #endif
                          {MENU_EXIT,         "EXIT",        NULL,                               (osdKeyFuncPtr)exitVtxMenu},
                          {MENU_SAVE_EXIT,    "SAVE+EXIT",   NULL,                               (osdKeyFuncPtr)exitVtxMenu}};

#define MENUE_SIZE        (sizeof(osdMenue) / sizeof(osdMenue[0]))

#if defined(USE_VTX)
void printMenuValueVtx(uint8_t x, uint8_t y, uint8_t idx) {
  char buffer[20] = {0};

  switch (idx) {
    case MENU_BAND:
      if(tempBand) {
        memcpy(buffer, vtx_get_band_name(tempBand - 1), 8);
      } else {
        sprintf(buffer, "DIRECT F");
      }
      break;
    case MENU_CHANNEL:
      sprintf(buffer, "%1i   ", tempChannel);
      break;
    case MENU_FREQUENCY:
      if(tempBand) {
        sprintf(buffer, "%1i",vtx_get_frequency(tempBand - 1, tempChannel - 1) );
      } else {
        sprintf(buffer, "%1i",vtx_get_config()->frequency);
      }
      break;
    case MENU_POWER:
      sprintf(buffer, "%i MW  ",vtx_get_power_mw() );
      break;
    case MENU_PIT_MODE:
      if (vtx_get_config()->pitmode)
        sprintf(buffer, "ON ");
      else
        sprintf(buffer, "OFF");
      break;
    default:
      break;
  }
  canvas_print(x, y, buffer);
  canvas_char_draw_complete();
}

void changeChannel(ButtonEvent_e btn, uint8_t idx) {
  switch (idx) {
    case MENU_BAND:
       if (btn == BTN_RIGHT)
        tempBand = ((tempBand) % vtx_get_band_count()) + 1;
      else
        tempBand = ((vtx_get_band_count() + tempBand - 2 ) % vtx_get_band_count()) + 1;
      break;
    case MENU_CHANNEL:
      if (btn == BTN_RIGHT)
        tempChannel = ((tempChannel ) % 8) + 1;
      else
        tempChannel = ((8 + tempChannel - 2) % 8) + 1;
      break;
    default:
      break;
  }
}

void changePower(ButtonEvent_e btn, uint8_t __attribute__((unused)) idx) {
  uint8_t power;

  if (btn == BTN_RIGHT)
    power = ((vtx_get_config()->power) % rf_pa_power_count()) + 1;
  else
    power = ((rf_pa_power_count() + vtx_get_config()->power - 2) % rf_pa_power_count()) + 1;
  vtx_set_power(power);
}

void changePit(ButtonEvent_e __attribute__((unused)) btn, uint8_t __attribute__((unused)) idx) {
  
  vtx_set_pitmode(1 - vtx_get_config()->pitmode);
  TRACE_INFO("pitmode %i\r",vtx_get_config()->pitmode);
}
#endif

void printMenuValue(uint8_t x, uint8_t y, uint8_t idx) {
  char buffer[20] = {0};

  switch (idx) {
    case MENU_DISPLAYPORT:
      if (settings.displayportEnabled)
        sprintf(buffer, "ON ");
      else
        sprintf(buffer, "OFF");
      break;
    case MENU_VIDEO_INPUT:
      if (tempVideoInput == 0)
        sprintf(buffer, "INPUT 1 ");
      else if (tempVideoInput == 1)
        sprintf(buffer, "INPUT 2 ");
      else
        sprintf(buffer, "CAM CTRL");
      break;
    default:
      break;
  }
  canvas_print(x, y, buffer);
  canvas_char_draw_complete();
}

void changeDisplayport(ButtonEvent_e __attribute__((unused)) btn, uint8_t __attribute__((unused)) idx) {
  settings.displayportEnabled = !settings.displayportEnabled;
}

void changeVideoIn(ButtonEvent_e __attribute__((unused)) btn, uint8_t __attribute__((unused)) idx) {
  if (btn == BTN_RIGHT)
    tempVideoInput = (tempVideoInput + 1) % 3;
  else
    tempVideoInput = (3 + tempVideoInput -1) % 3;
  
  if(tempVideoInput == 0) {
    set_video_input(0);
    settings.activeVideoInput = 0;
    settings.camswitchEnabled = false;
  } if(tempVideoInput == 1) {
    set_video_input(1);
    settings.activeVideoInput = 1;
    settings.camswitchEnabled = false;
  } if(tempVideoInput == 2) {
    set_video_input(fc.status.cameraControl1);
    settings.activeVideoInput = 0;
    settings.camswitchEnabled = true;
  }
}

void exitVtxMenu(ButtonEvent_e btn, uint8_t idx) {
  if (btn == BTN_RIGHT) {
    osdState = OSD_EXIT_MENU;
    if (idx == MENU_SAVE_EXIT) {
      #if defined(USE_VTX)
      vtx_set_band_channel(tempBand, tempChannel);
      #endif
      settings_save();
    }
  }
}


void msp_menu(void) {
  static ButtonEvent_e btn = BTN_INVALID;
  static ButtonEvent_e btnLast = BTN_INVALID;

  if      (fc.stickPos == 0x20)            btn = BTN_ENTER;
  else if (fc.stickPos == 0x10)            btn = BTN_EXIT;
  else if (fc.stickPos == 0x65)            btn = BTN_ENTER_VTX;
  else if ((fc.stickPos & 0x0f) == 0x00)   btn = BTN_MID;
  else if ((fc.stickPos & 0x0f) == 0x01)   btn = BTN_LEFT;
  else if ((fc.stickPos & 0x0f) == 0x02)   btn = BTN_RIGHT;
  else if ((fc.stickPos & 0x0f) == 0x04)   btn = BTN_DOWN;
  else if ((fc.stickPos & 0x0f) == 0x08)   btn = BTN_UP;
  else                                  btn = BTN_INVALID;

  static uint8_t selectedEntry = 0;

  if ((osdState == OSD_MSP || osdState == OSD_OFF) && (btn == BTN_ENTER_VTX) && !fc.status.armed) {
    osdState = OSD_MENU;
    selectedEntry = 0;
    btnLast = BTN_INVALID;
    #if defined(USE_VTX)
    tempChannel = vtx_get_config()->channel;
    tempBand = vtx_get_config()->band;
    #endif
    show_logo = false;

    if (settings.camswitchEnabled)
      tempVideoInput = 2;
    else
      tempVideoInput = settings.activeVideoInput;

    TRACE_INFO("osdState = OSD_VTX %i\n", osdState);

    setSyncMode(AUTOMATIC);
    canvas_char_clean();
    for (uint8_t i = 0; i < MENUE_SIZE; i++) {
      canvas_print(OSD_MENU_TEXT_LEFT, OSD_MENU_TOP + i, osdMenue[i].text);
      if (i == selectedEntry)
        canvas_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + i, ">");
      if (osdMenue[i].printFunc != NULL) {
        osdMenue[i].printFunc(OSD_MENU_VALUE_LEFT ,OSD_MENU_TOP + i, osdMenue[i].idx);
        if (osdMenue[i].keyFunc != NULL) {
          canvas_print(OSD_MENU_VALUE_LEFT - 2, OSD_MENU_TOP + i, "<");
          canvas_print(OSD_MENU_VALUE_LEFT + 9, OSD_MENU_TOP + i, ">");
        }
      }
    }
    canvas_char_draw_complete();
    
  }

  if ((osdState == OSD_MENU && fc.status.armed) || (osdState == OSD_EXIT_MENU)) {
    TRACE_INFO("osdState = OSD_MSP\n");
    canvas_char_clean();
    canvas_char_draw_complete();
    
    if (settings.displayportEnabled) {
      setSyncMode(AUTOMATIC);
      osdState = OSD_MSP;
    } else {
      setSyncMode(OFF);
      osdState = OSD_OFF;
    }
  }

  if (osdState != OSD_MENU) {
    btnLast = btn;
    return;
  }

  if (btnLast == BTN_MID && (btn == BTN_LEFT || btn == BTN_RIGHT)) {
    if (osdMenue[selectedEntry].keyFunc != NULL) {
      osdMenue[selectedEntry].keyFunc(btn, osdMenue[selectedEntry].idx);
      for (uint8_t i = 0; i < MENUE_SIZE; i++) {
        if (osdState == OSD_MENU && osdMenue[i].printFunc != NULL)
          osdMenue[i].printFunc(OSD_MENU_VALUE_LEFT ,OSD_MENU_TOP + i, osdMenue[i].idx);
      }
    }
  }

  if (btnLast == BTN_MID && (btn == BTN_DOWN || btn == BTN_UP)) {
    canvas_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + selectedEntry, " ");
    if (btn == BTN_DOWN)
      selectedEntry = (selectedEntry + 1) % MENUE_SIZE;
    else
      selectedEntry = (MENUE_SIZE + selectedEntry - 1) % MENUE_SIZE;
    canvas_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + selectedEntry, ">");
    canvas_char_draw_complete();
  }

  btnLast = btn;
}