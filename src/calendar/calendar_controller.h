/**
 * @file calendar_controller.h
 * @brief Calendar controller for fetching iCal events and setting alarms.
 */

#pragma once
#include "alarm_setter.h"
#include "calendar_model.h"

#include <RTClib.h>


class Calendar_view;

class Calendar_controller
{
  public:
  Calendar_controller(Calendar_model* _model, Calendar_view* _view, Alarm_setter* _alarm_controller);
  bool fetch_events(const DateTime& now);
  bool fetch_alarms(const DateTime& now);
  void update_view();

  private:
  bool fetch_ical(const String& url, bool is_alarm, const DateTime& now);
  Calendar_model* model;
  Calendar_view* view;
  Alarm_setter* alarm_controller;
};
