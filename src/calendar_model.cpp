#include "calendar_model.h"

void Calendar_model::clear()
{
  calendar_events.clear();
}

void Calendar_model::update(const Calendar_event& calendar_event)
{
  calendar_events.push_back(calendar_event);
}

uint8_t Calendar_model::get_event_count() const
{
  return calendar_events.size();
}

void Calendar_model::get_event(Calendar_event& calendar_event, uint8_t event_number) const
{
  calendar_event = calendar_events[event_number];
}

void Calendar_model::set_config(google_api_config& _config)
{
  config = _config;
}

void Calendar_model::get_config(google_api_config& _config) const
{
  _config = config;
}