#include "clock_model.h"

void Clock_model::set_wifi_config(Wifi_Config& _config)
{
  wifi_config = _config;
}

void Clock_model::get_wifi_config(Wifi_Config& _config) const
{
  _config = wifi_config;
}