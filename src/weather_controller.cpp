#include "weather_controller.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

Weather_controller::Weather_controller(Weather_model* m, Weather_view* v)
: model(m)
, view(v)
{}

void Weather_controller::fetch_weather(DateTime& now)
{
  for (size_t i = 0; i < 4; i++)
  {
    String dateStr = get_date_string(now, i);

    Open_weather_config config;
    model->get_open_weather_config(config);

    Serial.println(config.api_key);
    String serverPath = "https://api.openweathermap.org/data/3.0/onecall/day_summary?lat=" + String(config.lat, 4) +
                        "&lon=" + String(config.lon, 4) + "&date=" + dateStr + "&appid=" + config.api_key + "&units=metric";

    HTTPClient http;
    http.begin(serverPath.c_str());
    int code = http.GET();

    if (code == HTTP_CODE_OK)
    {
      String response = http.getString();

      StaticJsonDocument<4096> docs;
      DeserializationError error = deserializeJson(docs, response);

      if (error)
      {
        Serial.print("Błąd JSON: ");
        Serial.println(error.c_str());
        http.end();
        return;
      }
      else
      {
        model->update(docs);
      }
    }
    else
    {
      Serial.print("Błąd HTTP: ");
      Serial.println(code);
    }
    http.end();
  }
}

void Weather_controller::update_view()
{
  //   Weather_model data = model->getData();
  //   view->show(model);
  // poszczegolne funkcje w view i odpowiednie dane z modelu (dni)
}

String Weather_controller::get_date_string(DateTime dt, uint8_t offset)
{
  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", dt.year(), dt.month(), (dt.day() + offset));
  return String(dateStr);
}