/**
 * @file weather_model.cpp
 * @brief Implementation of the weather model — storing forecast data and configuration.
 */

#include "weather_model.h"

/**
 * @brief Updates the forecast entry at the specified day index.
 * @param index Day index to update (0 = today).
 * @param forecast_weather Weather data to store at that index.
 */
void Weather_model::update_at(uint8_t index, const Simple_weather& forecast_weather)
{
  if (index < WEATHER_DAYS)
    forecast[index] = forecast_weather;
}

/**
 * @brief Copies the forecast for the specified day offset into the provided structure.
 * @param weather Reference to the structure that will receive the forecast data.
 * @param offset_days Day offset from today (0 = today).
 */
void Weather_model::get_forecast(Simple_weather& weather, uint8_t offset_days) const
{
  if (offset_days < WEATHER_DAYS)
    weather = forecast[offset_days];
  else
    weather = Simple_weather{};
}

/**
 * @brief Saves the OpenWeather API configuration in the model.
 * @param _config Reference to the configuration structure to store.
 */
void Weather_model::set_config(Open_weather_config& _config)
{
  config = _config;
}

/**
 * @brief Copies the OpenWeather API configuration into the provided structure.
 * @param _config Reference to the structure that will receive the configuration.
 */
void Weather_model::get_config(Open_weather_config& _config) const
{
  _config = config;
}

/**
 * @brief Sets the current part of the day used to select the displayed temperature.
 * @param part The part of the day (morning, afternoon, or evening).
 */
void Weather_model::set_day_part(Day_part part)
{
  day_part = part;
}

/**
 * @brief Returns the currently stored part of the day.
 * @return The current Day_part value.
 */
Day_part Weather_model::get_day_part() const
{
  return day_part;
}
