/**
 * @file weather_controller.cpp
 * @brief Implementation of the weather controller — fetching OpenWeather forecast and updating the model and view.
 */

#include "weather_controller.h"

#include "logger.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

/**
 * @brief Initializes the controller with pointers to the model, view, and HTTP server.
 * @param _model Pointer to the weather model.
 * @param _view Pointer to the weather view.
 * @param _http_server Pointer to the HTTP server used for Home Assistant weather queries.
 */
Weather_controller::Weather_controller(Weather_model* _model, Weather_view* _view, HttpServer* _http_server)
: model(_model)
, view(_view)
, http_server(_http_server)
{}

/**
 * @brief Fetches weather forecast for multiple days from OpenWeather API or Home Assistant and updates the model.
 * @param now Reference to the current RTC time used to calculate forecast dates.
 */
void Weather_controller::fetch_weather(DateTime& now)
{
  Open_weather_config cfg_check;
  model->get_config(cfg_check);
  if (cfg_check.api_key.isEmpty() && !http_server->is_weather_from_ha())
  {
    return;
  }

  for (size_t i = 0; i < WEATHER_DAYS; i++)
  {
    String dateStr = get_date_string(now, i);

    Open_weather_config config;
    model->get_config(config);

    String serverPath = "https://api.openweathermap.org/data/3.0/onecall/day_summary?lat=" + String(config.lat, 4) +
                        "&lon=" + String(config.lon, 4) + "&date=" + dateStr + "&appid=" + config.api_key + "&units=metric";

    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(10000);
    http.begin(serverPath.c_str());
    int code = http.GET();

    if (code != HTTP_CODE_OK)
    {
      Logger::error("OWM", "HTTP " + String(code) + " for day+" + String(i));
      http.end();
      continue;
    }

    {
      String response = http.getString();

      if (!response.startsWith("{"))
      {
        Logger::error("OWM", "Response is not JSON for day+" + String(i));
        http.end();
        continue;
      }

      StaticJsonDocument<4096> doc;
      DeserializationError error = deserializeJson(doc, response);

      if (error)
      {
        Logger::error("OWM", "JSON parse error: " + String(error.c_str()));
        http.end();
        continue;
      }
      else
      {
        Simple_weather forecast_weather;
        float temp;
        if (http_server->is_weather_from_ha() && i == 0)
        {
          forecast_weather.temperature_morning = http_server->get_ha_weather();
          forecast_weather.temperature_afternoon = http_server->get_ha_weather();
          forecast_weather.temperature_evening = http_server->get_ha_weather();
        }
        else
        {
          temp = doc["temperature"]["afternoon"];
          forecast_weather.temperature_afternoon = (int)round(temp);
          temp = doc["temperature"]["morning"];
          forecast_weather.temperature_morning = (int)round(temp);
          temp = doc["temperature"]["evening"];
          forecast_weather.temperature_evening = (int)round(temp);
        }
        forecast_weather.precipitation = doc["precipitation"]["total"];
        forecast_weather.cloud_cover = doc["cloud_cover"]["afternoon"];
        model->update_at(i, forecast_weather);
      }
    }

    http.end();
  }

  if (now.hour() >= config::day_part_morning_from)
  {
    night_fetched = false;
  }
  else if (!night_fetched && !http_server->is_weather_from_ha())
  {
    Open_weather_config config;
    model->get_config(config);
    if (!config.api_key.isEmpty())
    {
      String serverPath = "https://api.openweathermap.org/data/3.0/onecall/day_summary?lat=" + String(config.lat, 4) +
                          "&lon=" + String(config.lon, 4) + "&date=" + get_date_string(now, -1) +
                          "&appid=" + config.api_key + "&units=metric";

      HTTPClient http;
      http.setConnectTimeout(10000);
      http.setTimeout(10000);
      http.begin(serverPath.c_str());
      int code = http.GET();

      if (code == HTTP_CODE_OK)
      {
        String response = http.getString();
        if (response.startsWith("{"))
        {
          StaticJsonDocument<4096> doc;
          if (!deserializeJson(doc, response))
          {
            Simple_weather today;
            model->get_forecast(today, 0);
            float temp = doc["temperature"]["night"];
            today.temperature_night = (int8_t)round(temp);
            model->update_at(0, today);
            night_fetched = true;
          }
        }
      }
      else
      {
        Logger::error("OWM", "HTTP " + String(code) + " for yesterday night");
      }
      http.end();
    }
  }

  check_day_part(now);
}

/**
 * @brief Refreshes the weather view based on the current model state.
 */
void Weather_controller::update_view()
{
  view->show(*model);
}

/**
 * @brief Returns the date in "YYYY-MM-DD" format offset by the given number of days.
 * @param dt Base date.
 * @param offset Number of days to add to the base date.
 * @return Date string in "YYYY-MM-DD" format.
 */
String Weather_controller::get_date_string(DateTime dt, int offset)
{
  DateTime dt_sum = offset >= 0 ? dt + TimeSpan(offset, 0, 0, 0) : dt - TimeSpan(-offset, 0, 0, 0);
  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", dt_sum.year(), dt_sum.month(), dt_sum.day());
  return String(dateStr);
}

/**
 * @brief Determines the current part of the day and updates the model accordingly.
 * @param now Reference to the current RTC time.
 */
void Weather_controller::check_day_part(DateTime& now)
{
  if (now.hour() < config::day_part_morning_from)
  {
    model->set_day_part(Day_part::night);
  }
  else if (now.hour() > config::day_part_afternoon_until)
  {
    model->set_day_part(Day_part::evening);
  }
  else if (now.hour() > config::day_part_morning_until)
  {
    model->set_day_part(Day_part::afternoon);
  }
  else
  {
    model->set_day_part(Day_part::morning);
  }
}