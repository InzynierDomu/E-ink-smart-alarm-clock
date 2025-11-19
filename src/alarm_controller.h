#pragma once

#include "RTClib.h"
#include "alarm_model.h"
#include "alarm_view.h"

class Alarm_controller
{
  public:
  Alarm_controller(Alarm_model* _model, Alarm_view* _view);
  void set_alarm(Simple_time time);
  bool check_alarm(DateTime& now);
  void set_no_alarm();
  void toggle_alarm();
  void enable_alarm();
  void update_view();

  private:
  Alarm_model* model;
  Alarm_view* view;
};
