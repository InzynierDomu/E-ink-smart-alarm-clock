#pragma once
#include <ArduinoJson.h>
#include <vector>

struct Open_weather_config
{
  String api_key;
  float lat;
  float lon;
};

struct Simple_weather
{
  uint8_t temperature_morning;
  uint8_t temperature_afternoon;
  uint8_t temperature_evening;
  uint8_t cloud_cover;
  uint8_t precipitation;
};
class Weather_model
{
  public:
  void update(const Simple_weather& forecast_weather);
  void clear();
  void get_forecast(Simple_weather& weather, uint8_t offset_days = 0) const;
  void set_config(Open_weather_config& _config);
  void get_config(Open_weather_config& _config) const;

  private:
  std::vector<Simple_weather> forecast;
  Open_weather_config config;
};