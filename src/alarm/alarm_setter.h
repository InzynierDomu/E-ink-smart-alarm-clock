/**
 * @file alarm_setter.h
 * @brief Interface for alarm state management without LVGL dependency.
 */

#pragma once
#include "clock_model.h"

struct Alarm_setter
{
  virtual void set_no_alarm() = 0;
  virtual void set_alarm(Simple_time time) = 0;
  virtual void enable_alarm() = 0;
  virtual ~Alarm_setter() = default;
};
