// http_server.h
#pragma once
#include <WebServer.h>
#include "clock_model.h"
#include "weather_model.h"
#include "calendar_model.h"
#include "audio.h"

class HttpServer {
public:
    HttpServer(WebServer& server,
               Clock_model& clock_model,
               Weather_model& weather_model,
               Calendar_model& calendar_model,
               Audio& audio);

    void begin();

private:
    WebServer&    server_;
    Clock_model&  clock_model_;
    Weather_model& weather_model_;
    Calendar_model& calendar_model_;
    Audio&        audio_;

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
