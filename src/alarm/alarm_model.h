/**
 * @file alarm_model.h
 * @brief Alarm model storing the alarm time and its enabled state.
 */

#pragma once
#include "clock_model.h"
#include "RTClib.h"

#include <Arduino.h>
#include <vector>

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
  void add_alarm(const Clock_alarm& alarm);
  void sort_alarms();
  void advance_alarm();
  void drop_past(uint8_t hour, uint8_t minute);
  void get_alarm(Clock_alarm& alarm) const;
  void set_no_alarm();
  bool get_is_alarm() const;
  void toggle_alarm();
  void enable_alarm();
  bool check_alarm(const DateTime& now) const;

  private:
  std::vector<Clock_alarm> alarms;
  bool is_alarm;
};
