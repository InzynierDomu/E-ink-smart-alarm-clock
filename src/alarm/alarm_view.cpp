/**
 * @file alarm_view.cpp
 * @brief Implementation of the alarm view — displaying alarm time and state via LVGL.
 */

#include "alarm_view.h"

/**
 * @brief Initializes the view with a pointer to the screen object.
 * @param scr Pointer to the Screen object.
 */
Alarm_view::Alarm_view(Screen* scr)
: screen(scr)
{}

/**
 * @brief Updates LVGL labels according to the current state of the alarm model.
 * @param data Reference to the alarm model containing data to display.
 */
void Alarm_view::show(const Alarm_model& data)
{
  if (data.get_is_alarm())
  {
    Clock_alarm alarm;
    data.get_alarm(alarm);
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", alarm.time.hour, alarm.time.minutes);
    lv_label_set_text(ui_labAlarm, time_str);
    if (alarm.enable)
    {
      lv_label_set_text(ui_labAlarmEnable, "E");
    }
    else
    {
      lv_label_set_text(ui_labAlarmEnable, "F");
    }
  }
  else
  {
    lv_label_set_text(ui_labAlarm, "00:00");
    lv_label_set_text(ui_labAlarmEnable, "F");
  }
}