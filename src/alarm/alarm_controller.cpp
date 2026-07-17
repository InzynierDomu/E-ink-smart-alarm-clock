/**
 * @file alarm_controller.cpp
 * @brief Implementation of the alarm controller — logic for setting, checking, and toggling the alarm.
 */

#include "alarm_controller.h"

/**
 * @brief Initializes the controller with pointers to the model and view.
 * @param _model Pointer to the alarm model.
 * @param _view Pointer to the alarm view.
 */
Alarm_controller::Alarm_controller(Alarm_model* _model, Alarm_view* _view)
: model(_model)
, view(_view)
{}

/**
 * @brief Sets the alarm time and activates it in the model.
 * @param time Alarm time as a Simple_time value.
 */
void Alarm_controller::set_alarm(Simple_time time)
{
  Clock_alarm alarm;
  alarm.time = time;
  alarm.enable = true;
  model->add_alarm(alarm);
}

/**
 * @brief Checks whether the current time matches the configured alarm time.
 * @param now Reference to the current RTC time.
 * @return true if the alarm is enabled and the current time matches, false otherwise.
 */
bool Alarm_controller::check_alarm(const DateTime& now)
{
  return model->check_alarm(now);
}

/**
 * @brief Resets the alarm — disables it and clears the configured time.
 */
void Alarm_controller::set_no_alarm()
{
  model->set_no_alarm();
}

/**
 * @brief Toggles the alarm enabled/disabled state if an alarm is set.
 */
void Alarm_controller::toggle_alarm()
{
  if (model->get_is_alarm())
  {
    model->toggle_alarm();
  }
}

/**
 * @brief Enables the alarm in the model without changing the configured time.
 */
void Alarm_controller::enable_alarm()
{
  model->enable_alarm();
}

/**
 * @brief Returns the time of the first (active) alarm.
 * @param out Set to the alarm time when returning true.
 * @return true if an alarm is currently set, false if none.
 */
bool Alarm_controller::get_alarm(Simple_time& out) const
{
  Clock_alarm a;
  model->get_alarm(a);
  if (!model->get_is_alarm())
    return false;
  out = a.time;
  return true;
}

/**
 * @brief Removes the first (active) alarm and advances to the next one if available.
 */
void Alarm_controller::advance_alarm()
{
  model->advance_alarm();
}

/**
 * @brief Refreshes the alarm view based on the current model state.
 */
void Alarm_controller::update_view()
{
  view->show(*model);
}
