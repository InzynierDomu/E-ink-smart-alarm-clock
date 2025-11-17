#include "clock_view.h"

Clock_view::Clock_view(Screen* scr)
: screen(scr)
{}

void Clock_view::show_time(DateTime& now)
{
  char buf[5];
  sprintf(buf, "%02d:%02d", now.hour(), now.minute());
  lv_label_set_text(ui_labtime, buf);
}

void Clock_view::show_date(const char* dateStr, uint8_t offset)
{
  lv_label_set_text(calendar_labels[offset], dateStr);
}

void Clock_view::setup_calendar_list()
{
  calendar_labels.push_back(ui_labDate);
  calendar_labels.push_back(ui_labDateDay1);
  calendar_labels.push_back(ui_labDateDay2);
  calendar_labels.push_back(ui_labDateDay3);
}