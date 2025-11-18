#include "weather_controller.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

Weather_controller::Weather_controller(Weather_model* _model, Weather_view* _view)
: model(_model)
, view(_view)
{}

void Weather_controller::fetch_weather(DateTime& now)
{
  model->clear();
  for (size_t i = 0; i < 4; i++)
  {
    String dateStr = get_date_string(now, i);

    Open_weather_config config;
    model->get_config(config);

    String serverPath = "https://api.openweathermap.org/data/3.0/onecall/day_summary?lat=" + String(config.lat, 4) +
                        "&lon=" + String(config.lon, 4) + "&date=" + dateStr + "&appid=" + config.api_key + "&units=metric";

    HTTPClient http;
    http.begin(serverPath.c_str());
    int code = http.GET();

    if (code == HTTP_CODE_OK)
    {
      String response = http.getString();

      StaticJsonDocument<4096> doc;
      DeserializationError error = deserializeJson(doc, response);

      if (error)
      {
        Serial.print("JSON error: ");
        Serial.println(error.c_str());
        http.end();
        return;
      }
      else
      {
        Simple_weather forecast_weather;
        float temp = doc["temperature"]["afternoon"];
        forecast_weather.temperature_afternoon = (int)round(temp);
        temp = doc["temperature"]["morning"];
        forecast_weather.temperature_morning = (int)round(temp);
        temp = doc["temperature"]["evening"];
        forecast_weather.temperature_evening = (int)round(temp);
        forecast_weather.precipitation = doc["precipitation"]["total"];
        forecast_weather.cloud_cover = doc["cloud_cover"]["afternoon"];
        model->update(forecast_weather);
      }
    }
    else
    {
      Serial.print("HTTP error: ");
      Serial.println(code);
    }
    http.end();
  }
}

void Weather_controller::update_view()
{
  view->show(*model);
}

String Weather_controller::get_date_string(DateTime dt, uint8_t offset)
{
  DateTime dt_sum = dt + TimeSpan(offset, 0, 0, 0);
  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", dt_sum.year(), dt_sum.month(), dt_sum.day());
  return String(dateStr);
}