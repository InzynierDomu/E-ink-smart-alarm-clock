/**
 * @file alarm_controller.h
 * @brief Alarm controller managing the logic for setting, checking, and toggling the alarm.
 */

#pragma once

#include "RTClib.h"
#include "alarm_model.h"
#include "alarm_setter.h"
#include "alarm_view.h"

class Alarm_controller : public Alarm_setter
{
  public:
  Alarm_controller(Alarm_model* _model, Alarm_view* _view);
  void set_alarm(Simple_time time) override;
  bool check_alarm(const DateTime& now);
  void set_no_alarm() override;
  void toggle_alarm();
  void enable_alarm() override;
  void advance_alarm();
  void update_view();

  private:
  Alarm_model* model;
  Alarm_view* view;
};
