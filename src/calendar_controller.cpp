#include "calendar_controller.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

Calendar_controller::Calendar_controller(Calendar_model* _model, Calendar_view* _view, Alarm_controller* _alarm_controller)
: model(_model)
, view(_view)
, alarm_controller(_alarm_controller)
{}

void Calendar_controller::fetch_calendar()
{
  HTTPClient http;
  http.setTimeout(20000);
  google_api_config config;
  model->get_config(config);

  // New API URL using api_base_url and device_id
  String url = config.api_base_url + "calendar.php?device_id=" + config.device_id;

  Serial.print("Fetching calendar from: ");
  Serial.println(url);

  if (!http.begin(url))
  {
    Serial.println("Cannot connect to Google Calendar API");
    return;
  }

  int httpResponseCode = http.GET();

  if (httpResponseCode == 301 || httpResponseCode == 302)
  {
    String newUrl = http.getLocation();
    Serial.printf("Przekierowanie: %d -> %s
", httpResponseCode, newUrl.c_str());
    http.end();

    http.begin(newUrl);
    httpResponseCode = http.GET();
  }

  if (httpResponseCode == 200)
  {
    String response = http.getString();
    Serial.println("JSON received:");

    DynamicJsonDocument doc(8192); // Zwiększ jeśli potrzebujesz
    DeserializationError error = deserializeJson(doc, response);
    if (!error && doc["success"])
    {
      model->clear();
      JsonArray events = doc["events"];
      bool is_alarm = false;
      if (!doc.containsKey("events") || !doc["events"].is<JsonArray>())
      {
        return;
      }
      for (JsonObject event : events)
      {
        String name = event["name"];
        String calendar = event["calendar"];
        String startStr = event["start"];
        String endStr = event["end"];

        Simple_time time_start(startStr);
        Simple_time time_stop(endStr);

        Calendar_event calendar_event(name, calendar, time_start, time_stop);
        model->update(calendar_event);

        if (calendar == "Alarm")
        {
          alarm_controller->set_alarm(time_start);
          is_alarm = true;
        }
      }
      if (!is_alarm)
      {
        // alarm_controller->clear_alarm();
      }
    }
    else
    {
      Serial.print("Błąd parsowania JSON: ");
      Serial.println(error.c_str());
    }
  }
  else
  {
    Serial.print("HTTP Error: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void Calendar_controller::update_view()
{
  view->clear_calendar_list();
  for (size_t i = 0; i < model->get_event_count(); ++i)
  {
    Calendar_event event;
    model->get_event(event, i);
    view->add_event(event.get_calendar_label());
  }
}
