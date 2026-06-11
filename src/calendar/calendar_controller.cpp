/**
 * @file calendar_controller.cpp
 * @brief Implementation of iCal calendar fetching via proxy and updating the model and alarm.
 */

#include "calendar_controller.h"

#include "calendar_parser.h"
#include "calendar_view.h"
#include "logger.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>


static const char* PROXY_BASE_URL = "https://inzynierdomu.pl/calendar_proxy/calendar.php";

/**
 * @brief Initializes the controller with pointers to the model, view, and alarm controller.
 * @param _model Pointer to the calendar model.
 * @param _view Pointer to the calendar view.
 * @param _alarm_controller Pointer to the alarm controller.
 */
Calendar_controller::Calendar_controller(Calendar_model* _model, Calendar_view* _view, Alarm_setter* _alarm_controller)
: model(_model)
, view(_view)
, alarm_controller(_alarm_controller)
{}

/**
 * @brief Encodes a string using percent-encoding (URL encoding).
 * @param str The input string to encode.
 * @return The URL-encoded string.
 */
static String url_encode(const String& str)
{
  String encoded;
  for (size_t i = 0; i < str.length(); i++)
  {
    char c = str[i];
    if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~')
    {
      encoded += c;
    }
    else
    {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
      encoded += buf;
    }
  }
  return encoded;
}

/**
 * @brief Fetches and processes an iCal calendar through the HTTP proxy.
 * @param ical_url URL of the iCal calendar to fetch.
 * @param is_alarm true if the calendar is used for alarm setting, false for regular events.
 * @param now Current time used to find the next upcoming alarm (ignored when is_alarm is false).
 */
bool Calendar_controller::fetch_ical(const String& ical_url, bool is_alarm, const DateTime& now)
{
  static WiFiClientSecure wifiClient;
  wifiClient.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(9000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  String proxy_url = String(PROXY_BASE_URL) + "?url=" + url_encode(ical_url);

  const String tag = is_alarm ? "CAL_ALARM" : "CAL";

  if (!http.begin(wifiClient, proxy_url))
  {
    Logger::error(tag, "Cannot connect to proxy");
    return false;
  }

  int httpCode = http.GET();
  if (httpCode != 200)
  {
    Logger::error(tag, "Proxy HTTP " + String(httpCode));
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  std::vector<Calendar_event> events = parse_ical_json(payload);

  if (is_alarm)
    apply_alarm_response(*alarm_controller, events, now);
  else
    apply_event_response(*model, events);
  return true;
}

/**
 * @brief Fetches calendar events and updates the model.
 * @param now Current time (unused for events, kept for interface consistency).
 */
bool Calendar_controller::fetch_events(const DateTime& now)
{
  google_api_config config;
  model->get_config(config);
  if (config.ical_url.length() > 0)
    return fetch_ical(config.ical_url, false, now);
  return true;
}

/**
 * @brief Fetches alarm calendar and sets the next upcoming alarm.
 * @param now Current time used to select the next upcoming alarm.
 */
bool Calendar_controller::fetch_alarms(const DateTime& now)
{
  google_api_config config;
  model->get_config(config);
  if (config.ical_alarm_url.length() > 0)
    return fetch_ical(config.ical_alarm_url, true, now);
  return true;
}

/**
 * @brief Refreshes the calendar view based on the current model state.
 */
void Calendar_controller::update_view()
{
  view->show(*model);
}
