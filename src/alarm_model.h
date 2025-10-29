#pragma once

#include "clock_model.h"

#include <Arduino.h>

struct Clock_alarm
{
  Clock_alarm()
  : time{Simple_time(0, 0)}
  , enable(false)
  {}
  Simple_time time;
  bool enable;
};

class Alarm_model
{
  public:
  Alarm_model();
  void set_alarm(Clock_alarm& alarm, bool is_alarm);
  void get_alarm(Clock_alarm& alarm);
  void set_no_alarm();
  bool get_is_alarm();

  private:
  Clock_alarm alarm;
  bool is_alarm;
};
