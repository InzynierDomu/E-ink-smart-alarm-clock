#include "weather_model.h"

void Weather_model::update(const JsonDocument& doc)
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
  forecast.push_back(forecast_weather);
}

void Weather_model::clear()
{
  forecast.clear();
}

void Weather_model::get_forecast(Simple_weather& weather, uint8_t offset_days) const
{
  if (offset_days < 4)
  {
    weather = forecast[offset_days];
  }
}

void Weather_model::set_open_weather_config(Open_weather_config& _config)
{
  config = _config;
}

void Weather_model::get_open_weather_config(Open_weather_config& _config) const
{
  _config = config;
}