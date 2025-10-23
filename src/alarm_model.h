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