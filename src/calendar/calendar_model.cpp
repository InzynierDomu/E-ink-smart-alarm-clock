/**
 * @file calendar_model.cpp
 * @brief Implementation of the calendar model — managing the event list and configuration.
 */

#include "calendar_model.h"

/**
 * @brief Removes all stored calendar events.
 */
void Calendar_model::clear()
{
  calendar_events.clear();
}

/**
 * @brief Adds an event to the calendar event list.
 * @param calendar_event The event to add.
 */
void Calendar_model::update(const Calendar_event& calendar_event)
{
  calendar_events.push_back(calendar_event);
}

/**
 * @brief Returns the number of stored calendar events.
 * @return Number of events.
 */
uint8_t Calendar_model::get_event_count() const
{
  return calendar_events.size();
}

/**
 * @brief Copies the event at the specified index into the provided structure.
 * @param calendar_event Reference to the structure that will receive the event.
 * @param event_number Index of the event in the list.
 */
void Calendar_model::get_event(Calendar_event& calendar_event, uint8_t event_number) const
{
  calendar_event = calendar_events[event_number];
}

/**
 * @brief Saves the calendar API configuration.
 * @param _config Reference to the configuration structure.
 */
void Calendar_model::set_config(google_api_config& _config)
{
  config = _config;
}

/**
 * @brief Copies the calendar API configuration into the provided structure.
 * @param _config Reference to the structure that will receive the configuration.
 */
void Calendar_model::get_config(google_api_config& _config) const
{
  _config = config;
}