#include "alarm_view.h"

Alarm_view::Alarm_view(Screen* scr)
: screen(scr)
{}

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
      lv_label_set_text(ui_labAlarmEnable, "ON");
    }
    else
    {
      lv_label_set_text(ui_labAlarmEnable, "OFF");
    }
  }
  else
  {
    lv_label_set_text(ui_labAlarm, "00:00");
    lv_label_set_text(ui_labAlarmEnable, "OFF");
  }
}