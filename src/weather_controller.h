#pragma once
#include "RTClib.h"
#include "weather_model.h"
#include "weather_view.h"


class Weather_controller
{
  public:
  Weather_controller(Weather_model* model, Weather_view* view);
  void fetch_weather(DateTime& now);
  void update_view();

  private:
  String get_date_string(DateTime dt, uint8_t offset = 0);

  Weather_model* model;
  Weather_view* view;
};