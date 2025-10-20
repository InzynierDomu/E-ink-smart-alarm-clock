#pragma once

#include "config.h"
#include "lvgl.h"
#include "ui/ui.h"

#include <GxEPD2_BW.h>

class Screen
{
  public:
  Screen();
  void setup_screen();

  private:
  static void my_disp_flush(lv_display_t* disp, const lv_area_t* area, unsigned char* data);
  void epd_setup();
  static uint32_t my_tick(void);

  GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display;
  static uint8_t lvBuffer[2][config::lv_buffer];
};