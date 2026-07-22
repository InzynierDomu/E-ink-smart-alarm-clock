/**
 * @file calendar_view.cpp
 * @brief Implementation of the calendar view — updating LVGL labels with events.
 */

#include "calendar_view.h"

Calendar_view::Calendar_view(Screen* scr)
: screen(scr)
{}

static bool is_ongoing(const Calendar_event& ev, const DateTime& now)
{
  if (ev.is_all_day())
    return true;

  int now_min  = now.hour() * 60 + now.minute();
  int start_min = ev.time_start.hour * 60 + ev.time_start.minutes;
  int stop_min  = ev.time_stop.hour  * 60 + ev.time_stop.minutes;
  return now_min >= start_min && now_min < stop_min;
}

static void apply_highlight(lv_obj_t* label, bool active)
{
  if (active)
  {
    lv_obj_set_style_bg_color(label, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
  }
  else
  {
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
  }
}

void Calendar_view::show(const Calendar_model& data, const DateTime& now)
{
  size_t m = data.get_event_count();
  size_t n = calendar_labels.size();
  if (m > n)
    m = n;

  for (size_t i = 0; i < n; ++i)
  {
    lv_obj_t* label = calendar_labels[i];
    if (i < m)
    {
      Calendar_event event;
      data.get_event(event, i);
      lv_label_set_text(label, event.get_calendar_label().c_str());
      apply_highlight(label, highlight_ongoing_ && is_ongoing(event, now));
    }
    else
    {
      lv_label_set_text(label, "-");
      apply_highlight(label, false);
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
