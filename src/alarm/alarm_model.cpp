/**
 * @file alarm_model.cpp
 * @brief Implementation of the alarm model — storing and modifying alarm data.
 */

#include "alarm_model.h"

#include <algorithm>

Alarm_model::Alarm_model()
: is_alarm(false)
{}

void Alarm_model::add_alarm(const Clock_alarm& _alarm)
{
  alarms.push_back(_alarm);
  is_alarm = true;
}

void Alarm_model::sort_alarms()
{
  std::sort(alarms.begin(), alarms.end(), [](const Clock_alarm& a, const Clock_alarm& b) {
    if (a.time.hour != b.time.hour)
      return a.time.hour < b.time.hour;
    return a.time.minutes < b.time.minutes;
  });
}

void Alarm_model::advance_alarm()
{
  if (!alarms.empty())
    alarms.erase(alarms.begin());
  if (alarms.empty())
    is_alarm = false;
}

void Alarm_model::drop_past(uint8_t hour, uint8_t minute)
{
  alarms.erase(
    std::remove_if(alarms.begin(), alarms.end(), [hour, minute](const Clock_alarm& a) {
      return a.time.hour < hour || (a.time.hour == hour && a.time.minutes < minute);
    }),
    alarms.end());
  if (alarms.empty())
    is_alarm = false;
}

/**
 * @brief Copies the first alarm into the provided structure, or a default if empty.
 * @param _alarm Reference to the structure that will receive the alarm data.
 */
void Alarm_model::get_alarm(Clock_alarm& _alarm) const
{
  if (!alarms.empty())
    _alarm = alarms[0];
  else
    _alarm = Clock_alarm{};
}

/**
 * @brief Clears all alarms and disables the alarm flag.
 */
void Alarm_model::set_no_alarm()
{
  alarms.clear();
  is_alarm = false;
}

/**
 * @brief Returns whether an alarm is currently set.
 * @return true if at least one alarm is set, false otherwise.
 */
bool Alarm_model::get_is_alarm() const
{
  return is_alarm && !alarms.empty();
}

/**
 * @brief Toggles the enable flag of the first alarm.
 */
void Alarm_model::toggle_alarm()
{
  if (!alarms.empty())
    alarms[0].enable = !alarms[0].enable;
}

/**
 * @brief Sets the enable flag of the first alarm to true.
 */
void Alarm_model::enable_alarm()
{
  if (!alarms.empty())
    alarms[0].enable = true;
}

/**
 * @brief Returns true when the first alarm is enabled and its time matches now.
 * @param now Current RTC time.
 * @return true if alarm is enabled and hour/minute match, false otherwise.
 */
bool Alarm_model::check_alarm(const DateTime& now) const
{
  if (alarms.empty() || !alarms[0].enable)
    return false;
  return now.hour() == alarms[0].time.hour && now.minute() == alarms[0].time.minutes;
}
