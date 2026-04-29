#include "calendar_controller.h"
#include "logger.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

static const char* PROXY_BASE_URL = "https://inzynierdomu.pl/calendar_proxy/calendar.php";

Calendar_controller::Calendar_controller(Calendar_model* _model, Calendar_view* _view, Alarm_controller* _alarm_controller)
: model(_model)
, view(_view)
, alarm_controller(_alarm_controller)
{}

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

static Simple_time parse_hhmm(const String& hhmm)
{
  int colon = hhmm.indexOf(':');
  if (colon < 0)
    return Simple_time(0, 0);
  uint8_t hour    = hhmm.substring(0, colon).toInt();
  uint8_t minutes = hhmm.substring(colon + 1).toInt();
  return Simple_time(hour, minutes);
}

void Calendar_controller::fetch_ical(const String& ical_url, bool is_alarm)
{
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure();
  HTTPClient http;
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  String proxy_url = String(PROXY_BASE_URL) + "?url=" + url_encode(ical_url);

  const String tag = is_alarm ? "CAL_ALARM" : "CAL";

  if (!http.begin(wifiClient, proxy_url))
  {
    Logger::error(tag, "Cannot connect to proxy");
    return;
  }

  int httpCode = http.GET();
  if (httpCode != 200)
  {
    Logger::error(tag, "Proxy HTTP " + String(httpCode));
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  if (!payload.startsWith("[") && !payload.startsWith("{"))
  {
    Logger::error(tag, "Response is not JSON");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err)
  {
    Logger::error(tag, "JSON parse error: " + String(err.c_str()));
    return;
  }

  if (!doc.is<JsonArray>())
  {
    Logger::error(tag, "Expected JSON array from proxy");
    return;
  }

  for (JsonObject event : doc.as<JsonArray>())
  {
    String summary   = event["summary"].as<String>();
    String start_str = event["start"].as<String>();
    String end_str   = event["end"].as<String>();

    Simple_time time_start = parse_hhmm(start_str);
    Simple_time time_stop  = parse_hhmm(end_str);

    if (is_alarm)
    {
      alarm_controller->set_alarm(time_start);
      alarm_controller->enable_alarm();
      Serial.printf("Alarm set: %02d:%02d\n", time_start.hour, time_start.minutes);
    }
    else
    {
      String calName = "";
      Calendar_event ev(summary, calName, time_start, time_stop);
      model->update(ev);
    }
  }
}

void Calendar_controller::fetch_calendar()
{
  google_api_config config;
  model->get_config(config);

  model->clear();

  if (config.ical_url.length() > 0)
  {
    Serial.println("Fetching calendar via proxy...");
    fetch_ical(config.ical_url, false);
  }

  if (config.ical_alarm_url.length() > 0)
  {
    alarm_controller->set_no_alarm();
    Serial.println("Fetching alarm via proxy...");
    fetch_ical(config.ical_alarm_url, true);
  }
}

void Calendar_controller::update_view()
{
  view->show(*model);
}
