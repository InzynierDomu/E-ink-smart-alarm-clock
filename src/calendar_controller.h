#pragma once
#include "alarm_controller.h"
#include "calendar_model.h"
#include "calendar_view.h"

class Calendar_controller
{
  public:
  Calendar_controller(Calendar_model* _model, Calendar_view* _view, Alarm_controller* _alarm_controller);
  void fetch_calendar();
  void update_view();

  private:
  Calendar_model* model;
  Calendar_view* view;
  Alarm_controller* alarm_controller;
};
