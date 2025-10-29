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
    lv_label_set_text(ui_labAlarm, alarm.time.to_string().c_str());
  }
  else
  {
    lv_label_set_text(ui_labAlarm, "00:00");
    lv_label_set_text(ui_labAlarmEnable, "OFF");
  }
}