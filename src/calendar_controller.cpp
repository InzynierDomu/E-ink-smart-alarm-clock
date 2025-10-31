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
  String url = "https://script.google.com/macros/s/" + config.script_url + "/exec";
  if (!http.begin(url))
  {
    Serial.println("Cannot connect to google script");
  }

  int httpResponseCode = http.GET();

  if (httpResponseCode == 301 || httpResponseCode == 302)
  {
    String newUrl = http.getLocation(); // Pobierz nowy URL z nagłówka Location
    Serial.printf("Przekierowanie: %d -> %s\n", httpResponseCode, newUrl.c_str());
    http.end();

    // Wywołaj ponownie do nowego URL (uwaga: rekurencja lub pętla, ale ogranicz max ilość przekierowań)
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
      for (JsonObject event : events)
      {
        String name = event["title"] | "";
        String calendar_name = event["calendarName"] | "";
        String temp_time = event["startTime"] | "";
        Simple_time start(temp_time);
        temp_time = event["endTime"] | "";
        Simple_time end(temp_time);
        Calendar_event new_event(name, calendar_name, start, end);
        Serial.println(new_event.name);
        Serial.println(new_event.calendar);
        if (calendar_name == config.alarm_calendar_id)
        {
          alarm_controller->set_alarm(new_event.time_start);
          is_alarm = true;
        }
        else if (calendar_name == config.google_calendar_id)
        {
          model->update(new_event);
        }
      }
      if (!is_alarm)
      {
        alarm_controller->set_no_alarm();
      }
    }
    else
    {
      Serial.println("JSON parse error lub brak wydarzeń");
    }
  }
  else
  {
    Serial.printf("HTTP error: %d\n", httpResponseCode);
  }
  http.end();
}

void Calendar_controller::update_view()
{
  view->show(*model);
}