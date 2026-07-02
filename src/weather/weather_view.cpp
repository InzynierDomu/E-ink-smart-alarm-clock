/**
 * @file weather_view.cpp
 * @brief Implementation of the weather view — updating LVGL labels with forecast temperatures and icons.
 */

#include "weather_view.h"

#include "weather_icon.h"

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

  static const uint8_t forecast_index[] = {0, 0, 1, 2};
  char temp_str[10];
  Simple_weather forecast;
  for (size_t i = 0; i < 4; ++i)
  {
    data.get_forecast(forecast, forecast_index[i]);
    bool is_night = (i == 0) && (data.get_day_part() == Day_part::night || data.get_day_part() == Day_part::night_next_day);
    lv_label_set_text(weatherIcons[i], weather_icon(forecast.cloud_cover, forecast.precipitation, is_night));
    if (i == 0)
    {
      Day_part part = data.get_day_part();
      switch (part)
      {
        case Day_part::night:
          sprintf(temp_str, "%d°", forecast.temperature_night);
          break;
        case Day_part::morning:
          sprintf(temp_str, "%d°", forecast.temperature_morning);
          break;
        case Day_part::afternoon:
          sprintf(temp_str, "%d°", forecast.temperature_afternoon);
          break;
        case Day_part::evening:
          sprintf(temp_str, "%d°", forecast.temperature_evening);
          break;
        case Day_part::night_next_day:
        {
          Simple_weather tomorrow;
          data.get_forecast(tomorrow, 1);
          sprintf(temp_str, "%d°", tomorrow.temperature_night);
          break;
        }
        default:
          sprintf(temp_str, "%d°", forecast.temperature_afternoon);
          break;
      }
      lv_label_set_text(ui_labTempMorning, temp_str);
    }
    else
    {
      sprintf(temp_str, "%d°", forecast.temperature_morning);
      lv_label_set_text(tempLabelsMorning[i], temp_str);
      sprintf(temp_str, "%d°", forecast.temperature_afternoon);
      lv_label_set_text(tempLabelsAfternoon[i], temp_str);
      sprintf(temp_str, "%d°", forecast.temperature_evening);
      lv_label_set_text(tempLabelsEvening[i], temp_str);
    }
  }
}

