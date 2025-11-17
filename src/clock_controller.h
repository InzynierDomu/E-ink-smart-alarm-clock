#pragma once

#include "RTClib.h"
#include "clock_model.h"
#include "clock_view.h"

class Clock_controller
{
  public:
  Clock_controller(Clock_view* _view, Clock_model* _model);
  void setup_clock();
  void get_time(DateTime& dt);
  void update_view();

  private:
  const char* get_date_string(DateTime dt, uint8_t offset);
  bool check_date_change();

  uint8_t last_day;
  Clock_view* view;
  Clock_model* model;
  RTC_DS1307 rtc; ///< DS1307 RTC
};
