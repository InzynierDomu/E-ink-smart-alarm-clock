// http_server.h
#pragma once
#include "audio.h"
#include "calendar_model.h"
#include "clock_model.h"
#include "weather_model.h"

#include <WebServer.h>


struct HA_config
{
  String ha_host;
  uint16_t ha_port;
  String ha_token;
  String ha_enitty_weather_name;
};

class HttpServer
{
  public:
  HttpServer(WebServer& server, Clock_model& clock_model, Weather_model& weather_model, Calendar_model& calendar_model, Audio& audio);

  void begin();
  void ha_set_config(HA_config& config);
  void get_ha_weather();

  private:
  WebServer& server_;
  Clock_model& clock_model_;
  Weather_model& weather_model_;
  Calendar_model& calendar_model_;
  Audio& audio_;
  HA_config ha_config;

  String buildPage();
  String buildWifiSection();
  String buildTimezoneSection();
  String buildWeatherSection();
  String buildAudioSection();

  void handleRoot();
  void handleSave();

  bool loadConfigJson(StaticJsonDocument<1024>& doc);
  bool saveConfigJson(const StaticJsonDocument<1024>& doc);
  void updateConfigFromRequest(StaticJsonDocument<1024>& doc);
};
