/**
 * @file calendar_parser.cpp
 * @brief Implementation of iCal JSON parsing: time string parsing and event extraction.
 */

#include "calendar_parser.h"
#include "logger.h"
#include <ArduinoJson.h>

Simple_time parse_hhmm(const String& hhmm)
{
  int colon = hhmm.indexOf(':');
  if (colon < 0)
    return Simple_time(0, 0);
  uint8_t h = (uint8_t)hhmm.substring(0, colon).toInt();
  uint8_t m = (uint8_t)hhmm.substring(colon + 1).toInt();
  return Simple_time(h, m);
}

std::vector<Calendar_event> parse_ical_json(const String& json)
{
  std::vector<Calendar_event> events;

  if (!json.startsWith("[") && !json.startsWith("{"))
  {
    Logger::error("CAL", "Response is not JSON");
    return events;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json.c_str());
  if (err)
  {
    Logger::error("CAL", "JSON parse error: " + String(err.c_str()));
    return events;
  }

  if (!doc.is<JsonArray>())
  {
    Logger::error("CAL", "Expected JSON array from proxy");
    return events;
  }

  for (JsonObject ev_obj : doc.as<JsonArray>())
  {
    const char* sum = ev_obj["summary"] | "";
    const char* st  = ev_obj["start"]   | "";
    const char* en  = ev_obj["end"]     | "";

    String summary(sum);
    String start_str(st);
    String end_str(en);
    String cal_name("");

    Simple_time time_start = parse_hhmm(start_str);
    Simple_time time_stop  = parse_hhmm(end_str);

    Calendar_event event(summary, cal_name, time_start, time_stop);
    events.push_back(event);
  }

  return events;
}
