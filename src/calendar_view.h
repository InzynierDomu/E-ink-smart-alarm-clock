#pragma once
#include "calendar_model.h"
#include "screen.h"

#include <vector>

class Calendar_view
{
  public:
  Calendar_view(Screen* scr);
  void show(const Calendar_model& data);
  void setup_calendar_list();

  private:
  Screen* screen;
  std::vector<lv_obj_t*> calendar_labels;
};
