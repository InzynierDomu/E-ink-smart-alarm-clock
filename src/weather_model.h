/**
 * @file weather_model.h
 * @brief Weather model storing multi-day forecast data and the OpenWeather API configuration.
 */

#pragma once
#include <ArduinoJson.h>

struct Open_weather_config
{
  String api_key;
  float lat;
  float lon;
};

enum class Day_part
{
  night,
  morning,
  afternoon,
  evening
};

struct Simple_weather
{
  int8_t temperature_night;
  int8_t temperature_morning;
  int8_t temperature_afternoon;
  int8_t temperature_evening;
  uint8_t cloud_cover;
  uint8_t precipitation;
};

static constexpr uint8_t WEATHER_DAYS = 4;

class Weather_model
{
  public:
  void update_at(uint8_t index, const Simple_weather& forecast_weather);
  void get_forecast(Simple_weather& weather, uint8_t offset_days = 0) const;
  void set_config(Open_weather_config& _config);
  void get_config(Open_weather_config& _config) const;
  void set_day_part(Day_part part);
  Day_part get_day_part() const;

  private:
  Simple_weather forecast[WEATHER_DAYS] = {};
  Open_weather_config config;
  Day_part day_part;
};