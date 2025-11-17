#pragma once

#include <Arduino.h>
#include <time.h>


struct Simple_time
{
  Simple_time() {}
  Simple_time(String& time)
  {
    hour = time.substring(11, 13).toInt();
    minutes = time.substring(14, 16).toInt();
  }
  Simple_time(uint8_t _hour, uint8_t _minutes)
  : hour(_hour)
  , minutes(_minutes)
  {}
  uint8_t hour;
  uint8_t minutes;

  String to_string() const
  {
    String h = hour < 10 ? "0" + String(hour) : String(hour);
    String m = minutes < 10 ? "0" + String(minutes) : String(minutes);
    return h + ":" + m;
  }
};

struct Wifi_Config
{
  String ssid;
  String pass;
  long timezone;
};


class Clock_model
{
  public:
  void set_wifi_config(Wifi_Config& _config);
  void get_wifi_config(Wifi_Config& _config) const;

  private:
  Wifi_Config wifi_config;
};
