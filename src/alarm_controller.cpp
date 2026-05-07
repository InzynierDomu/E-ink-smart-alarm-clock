#include "alarm_controller.h"

Alarm_controller::Alarm_controller(Alarm_model* _model, Alarm_view* _view)
: model(_model)
, view(_view)
{}

void Alarm_controller::add_alarm(Simple_time time)
{
  Clock_alarm alarm;
  alarm.time = time;
  alarm.enable = true;
  model->add_alarm(alarm);
}

void Alarm_controller::sort_alarms()
{
  model->sort_alarms();
}

void Alarm_controller::advance_alarm()
{
  model->advance_alarm();
}

void Alarm_controller::drop_past(const DateTime& now)
{
  model->drop_past(now.hour(), now.minute());
}

bool Alarm_controller::check_alarm(DateTime& now)
{
  Clock_alarm alarm;
  model->get_alarm(alarm);
  if (alarm.enable)
  {
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

void Alarm_controller::enable_alarm()
{
  model->enable_alarm();
}

void Alarm_controller::update_view()
{
  view->show(*model);
}
