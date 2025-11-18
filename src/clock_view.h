#pragma once
#include "RTClib.h"
#include "screen.h"

#include <vector>

class Clock_view
{
  public:
  Clock_view(Screen* scr);
  void show_time(DateTime& now);
  void show_date(const char* dateStr, uint8_t offset);
  void setup_calendar_list();

  private:
  Screen* screen;
  std::vector<lv_obj_t*> calendar_labels;
};
