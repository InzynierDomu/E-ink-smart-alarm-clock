/**
 * @file calendar_parser.cpp
 * @brief Implementation of iCal JSON parsing: time string parsing and event extraction.
 */

#include "calendar_parser.h"

#include "logger.h"

#include <ArduinoJson.h>


/**
 * @brief Parses a time string in "HH:MM" format into a Simple_time structure.
 * @param hhmm String containing the time in "HH:MM" format.
 * @return Parsed time as Simple_time; (0,0) on parse error.
 */
Simple_time parse_hhmm(const String& hhmm)
{
  int colon = hhmm.indexOf(':');
  if (colon < 0)
    return Simple_time(0, 0);
  uint8_t h = (uint8_t)hhmm.substring(0, colon).toInt();
  uint8_t m = (uint8_t)hhmm.substring(colon + 1).toInt();
  return Simple_time(h, m);
}

/**
 * @brief Parses a JSON array of calendar events returned by the iCal proxy.
 * @param json JSON string — expected to be an array of objects with "summary", "start", "end" fields.
 * @return Vector of parsed Calendar_event; empty on any error or if input is empty.
 */
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
    const char* st = ev_obj["start"] | "";
    const char* en = ev_obj["end"] | "";

    String summary(sum);
    String start_str(st);
    String end_str(en);
    String cal_name("");

    Simple_time time_start = parse_hhmm(start_str);
    Simple_time time_stop = parse_hhmm(end_str);

    Calendar_event event(summary, cal_name, time_start, time_stop);
    events.push_back(event);
  }

  return events;
}

bool select_next_alarm(const std::vector<Calendar_event>& events, const DateTime& now, Simple_time& out)
{
  int now_total = now.hour() * 60 + now.minute();
  int best_total = -1;

  for (const auto& ev : events)
  {
    int ev_total = ev.time_start.hour * 60 + ev.time_start.minutes;
    if (ev_total > now_total && (best_total < 0 || ev_total < best_total))
    {
      best_total = ev_total;
      out = ev.time_start;
    }
  }

  return best_total >= 0;
}

void apply_event_response(Calendar_model& model, const std::vector<Calendar_event>& events)
{
  model.clear();
  for (const auto& ev : events)
    model.update(ev);
}

void apply_alarm_response(Alarm_setter& alarm, const std::vector<Calendar_event>& events, const DateTime& now)
{
  alarm.set_no_alarm();
  Simple_time next(0, 0);
  if (select_next_alarm(events, now, next))
  {
    alarm.set_alarm(next);
    alarm.enable_alarm();
  }
}
