/**
 * @file weather_view.h
 * @brief Weather view for rendering forecast temperatures and weather icons via LVGL.
 */

#pragma once
#include "screen.h"
#include "weather_model.h"

class Weather_view
{
  public:
  Weather_view(Screen* scr);
  void show(const Weather_model& data);

  private:
  Screen* screen;
};