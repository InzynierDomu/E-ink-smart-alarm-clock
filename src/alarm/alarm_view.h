/**
 * @file alarm_view.h
 * @brief View class responsible for displaying the alarm state on the screen.
 */

#pragma once
#include "alarm_model.h"
#include "screen.h"

class Alarm_view
{
  public:
  Alarm_view(Screen* scr);
  void show(const Alarm_model& data);

  private:
  Screen* screen;
};