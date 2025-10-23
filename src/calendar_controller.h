#pragma once
#include "calendar_model.h"
#include "calendar_view.h"

class Calendar_controller
{
  public:
  Calendar_controller(Calendar_model* _model, Calendar_view* view);
  void fetch_calendar();
  void update_view();

  private:
  Calendar_model* model;
  Calendar_view* view;
};
