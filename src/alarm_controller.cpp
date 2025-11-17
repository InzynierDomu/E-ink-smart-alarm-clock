#include "alarm_controller.h"

Alarm_controller::Alarm_controller(Alarm_model* _model, Alarm_view* _view)
: model(_model)
, view(_view)
{}

void Alarm_controller::set_alarm(Simple_time time)
{
  Clock_alarm alarm;
  alarm.time = time;
  model->set_alarm(alarm, true);
}

bool Alarm_controller::check_alarm(DateTime& now)
{
  Clock_alarm alarm;
  model->get_alarm(alarm);
  if (alarm.enable)
  {
    Serial.println(alarm.time.hour);
    if (now.hour() == alarm.time.hour && now.minute() == alarm.time.minutes)
    {
      return true;
    }
  }
  return false;
}

void Alarm_controller::set_no_alarm()
{
  model->set_no_alarm();
}

void Alarm_controller::toggle_alarm()
{
  if (model->get_is_alarm())
  {
    model->toggle_alarm();
  }
}

void Alarm_controller::update_view()
{
  view->show(*model);
}
