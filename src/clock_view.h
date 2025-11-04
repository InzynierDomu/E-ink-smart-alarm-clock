#pragma once

#include "RTClib.h"
#include "Screen.h"

class Clock_view
{
  public:
  Clock_view(Screen* scr);
  void show(DateTime& now);

  private:
};
