/**
 * @file calendar_view.h
 * @brief Calendar view for displaying the event list on the e-ink screen via LVGL.
 */

#pragma once
#include "calendar_model.h"
#include "screen.h"

#include <RTClib.h>
#include <vector>

class Calendar_view
{
  public:
  Calendar_view(Screen* scr);
  void show(const Calendar_model& data, const DateTime& now);
  void setup_calendar_list();
  void set_highlight_ongoing(bool enabled) { highlight_ongoing_ = enabled; }

  private:
  Screen* screen;
  std::vector<lv_obj_t*> calendar_labels;
  bool highlight_ongoing_ = false;
};
