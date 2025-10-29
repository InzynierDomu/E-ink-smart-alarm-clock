#pragma once
#include "Screen.h"
#include "alarm_model.h"

class Alarm_view
{
  public:
  Alarm_view(Screen* scr);
  void show(const Alarm_model& data);
  private:
  Screen* screen;
};