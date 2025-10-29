#pragma once
#include "calendar_model.h"
#include "calendar_view.h"
#include "alarm_model.h"
#include "alarm_view.h"

class Calendar_controller
{
  public:
  Calendar_controller(Calendar_model* _model, Alarm_model* _alarm_model, Calendar_view* _view, Alarm_view* _alarm_view);
  void fetch_calendar();
  void update_view();

  private:
    void convert_event_to_alarm(Calendar_event& event, Clock_alarm& alarm);

  Calendar_model* model;
  Alarm_model* alarm_model;
  Calendar_view* view;
  Alarm_view* alarm_view;
};
