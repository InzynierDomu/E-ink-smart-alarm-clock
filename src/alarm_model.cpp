#include "alarm_model.h"

Alarm_model::Alarm_model() {}

void Alarm_model::set_alarm(Clock_alarm& alarm, bool _is_alarm)
{
  is_alarm = _is_alarm;
}

void Alarm_model::get_alarm(Clock_alarm& _alarm) const
{
  _alarm = alarm;
}

void Alarm_model::set_no_alarm()
{
  is_alarm = false;
}

bool Alarm_model::get_is_alarm() const
{
  return is_alarm;
}