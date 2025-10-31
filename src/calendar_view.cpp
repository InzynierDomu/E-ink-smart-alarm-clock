#include "calendar_view.h"

Calendar_view::Calendar_view(Screen* scr)
: screen(scr)
{}

void Calendar_view::show(const Calendar_model& data)
{
  size_t m = data.get_event_count();
  size_t n = calendar_labels.size();
  if (m > n)
  {
    m = n;
  }

  for (size_t i = 0; i < n; ++i)
  {
    if (i < m)
    {
      Calendar_event event;
      data.get_event(event, i);
      String label = event.get_calendar_label();
      lv_label_set_text(calendar_labels[i], label.c_str());
    }
    else
    {
      lv_label_set_text(calendar_labels[i], "-");
    }
  }
}

void Calendar_view::setup_calendar_list()
{
  calendar_labels.push_back(ui_labCalendarEvent1);
  calendar_labels.push_back(ui_labCalendarEvent2);
  calendar_labels.push_back(ui_labCalendarEvent3);
  calendar_labels.push_back(ui_labCalendarEvent4);
  calendar_labels.push_back(ui_labCalendarEvent5);
  calendar_labels.push_back(ui_labCalendarEvent6);
  calendar_labels.push_back(ui_labCalendarEvent7);
}