/**
 * @file weather_view.cpp
 * @brief Implementation of the weather view — updating LVGL labels with forecast temperatures and icons.
 */

#include "weather_view.h"

#include "weather_icon.h"
#include "../logger.h"

static bool is_valid_temp(int8_t t)   { return t > -60 && t < 60; }
static bool is_valid_cloud(uint8_t c) { return c <= 100; }
static bool is_valid_precip(uint8_t p){ return p <= 200; }

static bool set_temp_label(lv_obj_t* label, int8_t temp)
{
  if (!label || !is_valid_temp(temp))
    return false;
  char buf[8];
  sprintf(buf, "%d°", temp);
  lv_label_set_text(label, buf);
  return true;
}

/**
 * @brief Initializes the view with a pointer to the screen object.
 * @param scr Pointer to the Screen object.
 */
Weather_view::Weather_view(Screen* scr)
: screen(scr)
{}

/**
 * @brief Updates all LVGL temperature labels and weather icon labels from the weather model.
 * @param data Reference to the weather model containing forecast data.
 */
void Weather_view::show(const Weather_model& data)
{
  static lv_obj_t* tempLabelsMorning[] = {ui_labTempMorning, ui_labTempMorningDay1, ui_labTempMorningDay2, ui_labTempMorningDay3};
  static lv_obj_t* tempLabelsAfternoon[] = {nullptr, ui_labTempAfternoonDay1, ui_labTempAfternoonDay2, ui_labTempAfternoonDay3};
  static lv_obj_t* tempLabelsEvening[] = {nullptr, ui_labTempEveningDay1, ui_labTempEveningDay2, ui_labTempEveningDay3};
  static lv_obj_t* weatherIcons[] = {ui_labWeatherIcon, ui_labWeatherIconDay1, ui_labWeatherIconDay2, ui_labWeatherIconDay3};
  static uint8_t bad_weather = 0;

  static const uint8_t forecast_index[] = {0, 0, 1, 2};
  bool had_garbage = false;
  Simple_weather forecast;
  for (size_t i = 0; i < 4; ++i)
  {
    data.get_forecast(forecast, forecast_index[i]);
    bool is_night = (i == 0) && (data.get_day_part() == Day_part::night || data.get_day_part() == Day_part::night_next_day);
    if (is_valid_cloud(forecast.cloud_cover) && is_valid_precip(forecast.precipitation))
      lv_label_set_text(weatherIcons[i], weather_icon(forecast.cloud_cover, forecast.precipitation, is_night));
    else
      had_garbage = true;

    if (i == 0)
    {
      Day_part part = data.get_day_part();
      int8_t temp_val = 0;
      switch (part)
      {
        case Day_part::night:          temp_val = forecast.temperature_night;     break;
        case Day_part::morning:        temp_val = forecast.temperature_morning;   break;
        case Day_part::afternoon:      temp_val = forecast.temperature_afternoon; break;
        case Day_part::evening:        temp_val = forecast.temperature_evening;   break;
        case Day_part::night_next_day:
        {
          Simple_weather tomorrow;
          data.get_forecast(tomorrow, 1);
          temp_val = tomorrow.temperature_night;
          break;
        }
        default: temp_val = forecast.temperature_afternoon; break;
      }
      if (!set_temp_label(ui_labTempMorning, temp_val))
        had_garbage = true;
    }
    else
    {
      if (!set_temp_label(tempLabelsMorning[i],   forecast.temperature_morning))   had_garbage = true;
      if (!set_temp_label(tempLabelsAfternoon[i], forecast.temperature_afternoon)) had_garbage = true;
      if (!set_temp_label(tempLabelsEvening[i],   forecast.temperature_evening))   had_garbage = true;
    }
  }

  if (had_garbage)
  {
    if (++bad_weather >= 3)
      Logger::error("WEATHER", "3 consecutive invalid weather data updates");
  }
  else
  {
    bad_weather = 0;
  }
}

