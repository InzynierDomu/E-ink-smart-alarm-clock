#pragma once
#include "clock_model.h"

#include <Arduino.h>
#include <vector>

struct google_api_config
{
  String script_url;
  String alarm_calendar_id;
  String google_calendar_id;
};

struct Calendar_event
{
  Calendar_event() {}
  Calendar_event(String& _name, String& _calendar, Simple_time& _time_start, Simple_time& _time_stop)
  : name(_name)
  , calendar(_calendar)
  , time_start(_time_start)
  , time_stop(_time_stop)
  {}
  String get_calendar_label()
  {
    String label = time_start.to_string() + " " + name;
    return label;
  }
  String name;
  String calendar;
  Simple_time time_start;
  Simple_time time_stop;
};

class Calendar_model
{
  public:
  void clear();
  void update(const Calendar_event& calendar_event);
  uint8_t get_event_count() const;
  void get_event(Calendar_event& calendar_event, uint8_t event_number) const;
  void set_config(google_api_config& _config);
  void get_config(google_api_config& _config) const;

  private:
  google_api_config config;
  std::vector<Calendar_event> calendar_events;
};