#include "weather_model.h"

void Weather_model::update_at(uint8_t index, const Simple_weather& forecast_weather)
{
  if (index < WEATHER_DAYS)
    forecast[index] = forecast_weather;
}

void Weather_model::get_forecast(Simple_weather& weather, uint8_t offset_days) const
{
  if (offset_days < WEATHER_DAYS)
    weather = forecast[offset_days];
  else
    weather = Simple_weather{};
}

void Weather_model::set_config(Open_weather_config& _config)
{
  config = _config;
}

void Weather_model::get_config(Open_weather_config& _config) const
{
  _config = config;
}

void Weather_model::set_day_part(Day_part part)
{
  day_part = part;
}

Day_part Weather_model::get_day_part() const
{
  return day_part;
}
