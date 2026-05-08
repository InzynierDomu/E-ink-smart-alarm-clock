/**
 * @file calendar_model.h
 * @brief Calendar model storing the list of events and API configuration.
 */

#pragma once
#include "clock_model.h"

#include <Arduino.h>
#include <vector>

/**
 * @brief Configuration for fetching calendar data via the iCal proxy.
 */
struct google_api_config
{
  String api_base_url;   ///< Base URL of the iCal proxy API.
  String device_id;      ///< Unique device identifier used for pairing with the proxy.
  String ical_url;       ///< iCal URL of the main Google Calendar.
  String ical_alarm_url; ///< iCal URL of the alarm Google Calendar.
};

/**
 * @brief Represents a single calendar event with a name, source calendar, and time range.
 */
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
  String name;            ///< Event title / summary.
  String calendar;        ///< Source calendar name.
  Simple_time time_start; ///< Event start time.
  Simple_time time_stop;  ///< Event end time.
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
