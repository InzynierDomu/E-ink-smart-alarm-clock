#include "weather_view.h"
// Dodać #include <lvgl.h> jeśli używasz LVGL

Weather_view::Weather_view(Screen* scr)
: screen(scr)
{
  // Tworzenie LVGL obiektów na screen
  // tempLabel = lv_label_create(screen->getLvObj());
}

void Weather_view::show(const Weather_model& data)
{
  static lv_obj_t* tempLabelsMorning[] = {ui_labTempMorning, ui_labTempMorningDay1, ui_labTempMorningDay2, ui_labTempMorningDay3};
  static lv_obj_t* tempLabelsAfternoon[] = {nullptr, ui_labTempAfternoonDay1, ui_labTempAfternoonDay2, ui_labTempAfternoonDay3};
  static lv_obj_t* tempLabelsEvening[] = {nullptr, ui_labTempEveningDay1, ui_labTempEveningDay2, ui_labTempEveningDay3};
  static lv_obj_t* weatherIcons[] = {ui_labWeatherIcon, ui_labWeatherIconDay1, ui_labWeatherIconDay2, ui_labWeatherIconDay3};

  char temp_str[10];
  Simple_weather forecast;
  for (size_t i = 0; i < 4; ++i)
  {
    data.get_forecast(forecast, i);
    if (tempLabelsMorning[i])
    {
      sprintf(temp_str, "%d℃", forecast.temperature_morning);
      lv_label_set_text(tempLabelsMorning[i], temp_str);
    }
    if (i > 0 && tempLabelsAfternoon[i])
    {
      sprintf(temp_str, "%d℃", forecast.temperature_afternoon);
      lv_label_set_text(tempLabelsAfternoon[i], temp_str);
      sprintf(temp_str, "%d℃", forecast.temperature_evening);
      lv_label_set_text(tempLabelsEvening[i], temp_str);
    }
    lv_label_set_text(weatherIcons[i], weather_icon_change(forecast.cloud_cover, forecast.precipitation));
  }

  sprintf(temp_str, "%d℃", forecast.temperature_afternoon);
  lv_label_set_text(ui_labTempMorning, temp_str);
}

const char* Weather_view::weather_icon_change(int cloud_cover, int precipitation)
{
  if (cloud_cover > 30 && precipitation == 0)
    return "ú";

  if (precipitation < 5 && precipitation != 0)
    return "û";

  if (precipitation > 5)
    return "ü";

  return "ù";
}