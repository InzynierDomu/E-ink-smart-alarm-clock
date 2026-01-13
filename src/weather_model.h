#pragma once
#include <ArduinoJson.h>
#include <vector>

struct Open_weather_config
{
  String api_key;
  float lat;
  float lon;
};

enum class Day_part
{
  morning,
  afternoon,
  evening
};

struct Simple_weather
{
  int8_t temperature_morning;
  int8_t temperature_afternoon;
  int8_t temperature_evening;
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
  void set_day_part(Day_part part);
  Day_part get_day_part() const;

  private:
  std::vector<Simple_weather> forecast;
  Open_weather_config config;
  Day_part day_part;
};