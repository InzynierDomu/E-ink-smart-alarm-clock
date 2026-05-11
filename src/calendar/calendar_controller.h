/**
 * @file calendar_controller.h
 * @brief Calendar controller for fetching iCal events and setting alarms.
 */

#pragma once
#include "alarm_controller.h"
#include "calendar_model.h"
#include "calendar_view.h"

class Calendar_controller
{
  public:
  Calendar_controller(Calendar_model* _model, Calendar_view* _view, Alarm_controller* _alarm_controller);
  void fetch_calendar(const DateTime& now);
  void update_view();

  private:
  void fetch_ical(const String& url, bool is_alarm, const DateTime& now);
  Calendar_model* model;
  Calendar_view* view;
  Alarm_controller* alarm_controller;
};
