#pragma once
#include "Screen.h"
#include "weather_model.h"

class Weather_view
{
  public:
  Weather_view(Screen* scr);
  void show(const Simple_weather& data);

  private:
  Screen* screen;
  // LVGL objects, np. lv_obj_t *tempLabel, *descLabel;
};