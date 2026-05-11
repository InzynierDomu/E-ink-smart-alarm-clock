/**
 * @file alarm_model.cpp
 * @brief Implementation of the alarm model — storing and modifying alarm data.
 */

#include "alarm_model.h"

Alarm_model::Alarm_model() {}

/**
 * @brief Sets the alarm data and its active flag.
 * @param _alarm Reference to the alarm structure containing time and state.
 * @param _is_alarm true if the alarm should be active.
 */
void Alarm_model::set_alarm(Clock_alarm& _alarm, bool _is_alarm)
{
  alarm = _alarm;
  is_alarm = _is_alarm;
}

/**
 * @brief Copies the currently configured alarm into the provided structure.
 * @param _alarm Reference to the structure that will receive the alarm data.
 */
void Alarm_model::get_alarm(Clock_alarm& _alarm) const
{
  _alarm = alarm;
}

/**
 * @brief Clears the alarm time and disables the alarm.
 */
void Alarm_model::set_no_alarm()
{
  alarm.time = Simple_time(0, 0);
  alarm.enable = false;
  is_alarm = false;
}

/**
 * @brief Returns whether an alarm is currently set.
 * @return true if an alarm is set, false otherwise.
 */
bool Alarm_model::get_is_alarm() const
{
  return is_alarm;
}

/**
 * @brief Toggles the alarm enable flag to its opposite value.
 */
void Alarm_model::toggle_alarm()
{
  alarm.enable = !alarm.enable;
}
/**
 * @brief Sets the alarm enable flag to true.
 */
void Alarm_model::enable_alarm()
{
  alarm.enable = true;
}

/**
 * @brief Returns true when the alarm is enabled and the current time matches the configured time.
 * @param now Current RTC time.
 * @return true if alarm is enabled and hour/minute match, false otherwise.
 */
bool Alarm_model::check_alarm(const DateTime& now) const
{
  if (!alarm.enable)
    return false;
  return now.hour() == alarm.time.hour && now.minute() == alarm.time.minutes;
}