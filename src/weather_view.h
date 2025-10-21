#pragma once
#include "Screen.h"
#include "weather_model.h"

class Weather_view
{
  public:
  Weather_view(Screen* scr);
  void show(const Weather_model& data);

  private:
    const char* weather_icon_change(int cloud_cover, int precipitation);

  Screen* screen;
  // LVGL objects, np. lv_obj_t *tempLabel, *descLabel;
};