// http_server.h
#pragma once
#include "audio.h"
#include "calendar_model.h"
#include "clock_model.h"
#include "weather_model.h"

#include <PubSubClient.h>
#include <WebServer.h>


struct HA_config
{
  String ha_host;
  uint16_t ha_port;
  String ha_token;
  String ha_user;
  String ha_pass;
  String ha_enitty_weather_name;
  String ha_entity_clock_name;
  uint16_t mqtt_port;
  bool weather_from_ha;
};

class HttpServer
{
  public:
  HttpServer(WebServer& server, Clock_model& clock_model, Weather_model& weather_model, Calendar_model& calendar_model, Audio& audio);

  void begin();
  void ha_set_config(HA_config& config);
  int8_t get_ha_weather();
  bool is_weather_from_ha();
  void entity_clock_setup();
  void send_mqtt_action();

  private:
  WebServer& server_;
  Clock_model& clock_model_;
  Weather_model& weather_model_;
  Calendar_model& calendar_model_;
  Audio& audio_;
  HA_config ha_config;
  WiFiClient client;
  PubSubClient mqtt_client;

  String buildPage();
  String buildWifiSection();
  String buildTimezoneSection();
  String buildWeatherSection();
  String buildGoogleCalendarSection();
  String buildAudioSection();
  String buildHaSection();
  String buildFooter();

  void handleRoot();
  void handleSave();

  bool loadConfigJson(StaticJsonDocument<1024>& doc);
  bool saveConfigJson(const StaticJsonDocument<1024>& doc);
  void updateConfigFromRequest(StaticJsonDocument<1024>& doc);
  void mqtt_reconnect();
};
