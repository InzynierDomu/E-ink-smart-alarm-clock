#include "RTClib.h"
#include "alarm_model.h"
#include "alarm_view.h"
#include "audio.h"
#include "calendar_controller.h"
#include "calendar_model.h"
#include "calendar_view.h"
#include "clock_controller.h"
#include "clock_model.h"
#include "clock_view.h"
#include "config.h"
#include "http_server.h"
#include "lvgl.h"
#include "screen.h"
#include "ui/ui.h"
#include "weather_controller.h"
#include "weather_model.h"
#include "weather_view.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <time.h>
#include <vector>

enum class State
{
  alarm,
  normal,
  AP
};

Screen screen;

SPIClass spi = SPIClass(HSPI);

Audio audio;

DynamicJsonDocument docs(19508);

Alarm_model alarm_model;
Alarm_view alarm_view(&screen);
Alarm_controller alarm_controller(&alarm_model, &alarm_view);

Calendar_model calendar_model;
Calendar_view calendar_view(&screen);
Calendar_controller calendar_controller(&calendar_model, &calendar_view, &alarm_controller);

Clock_model clock_model;
Clock_view clock_view(&screen);
Clock_controller clock_controller(&clock_view, &clock_model);

Weather_model weather_model;

WebServer server(80);
HttpServer httpServer(server, clock_model, weather_model, calendar_model, audio);

Weather_view weather_view(&screen);
Weather_controller weather_controller(&weather_model, &weather_view, &httpServer);

State state;
void read_config()
{
  File file = SD.open(config::config_path, "r");
  if (!file)
  {
    Serial.println("error, no config file");
    return;
  }

  String jsonData;
  while (file.available())
  {
    jsonData += (char)file.read();
  }
  file.close();

  StaticJsonDocument<1024> doc;

  DeserializationError error = deserializeJson(doc, jsonData);
  if (error)
  {
    Serial.print("Błąd parsowania JSON: ");
    Serial.println(error.c_str());
    return;
  }

  Wifi_Config wifi_config;
  wifi_config.ssid = doc["ssid"] | "";
  wifi_config.pass = doc["pass"] | "";
  wifi_config.timezone = doc["timezone"];
  clock_model.set_wifi_config(wifi_config);

  Open_weather_config weather_config;
  weather_config.api_key = doc["openweathermap_api_key"] | "";
  weather_config.lat = doc["lat"];
  weather_config.lon = doc["lon"];
  weather_model.set_config(weather_config);

  google_api_config google_config;
  google_config.script_url = doc["google_script_url"] | "";
  google_config.alarm_calendar_id = doc["google_calendar_alarm_id"] | "";
  google_config.google_calendar_id = doc["google_calendar_id"] | "";
  calendar_model.set_config(google_config);

  HA_config ha_config;
  ha_config.ha_host = doc["HA_host"] | "";
  ha_config.ha_port = doc["HA_port"];
  ha_config.ha_token = doc["HA_token"] | "";
  ha_config.ha_enitty_weather_name = doc["HA_weather_entity_name"] | "";
  ha_config.weather_from_ha = doc["weather_from_HA"];
  httpServer.ha_set_config(ha_config);

  Audio_config audio_config;
  audio_config.sample_rate = doc["sample_rate"];
  audio_config.volume = doc["volume"];
  audio.set_config(audio_config);
}

void update_clock()
{
  clock_controller.update_view();
  DateTime now;
  clock_controller.get_time(now);

  if (alarm_controller.check_alarm(now))
  {
    digitalWrite(config::led_pin, HIGH);
    state = State::alarm;
  }
}

static void update_date(lv_timer_t* timer)
{
  static int update_counter = 10;
  update_clock();
  if (update_counter >= 10)
  {
    DateTime now;
    clock_controller.get_time(now);
    weather_controller.fetch_weather(now);
    weather_controller.update_view();
    calendar_controller.fetch_calendar();
    calendar_controller.update_view();
    alarm_controller.update_view();
    httpServer.get_ha_weather();
    update_counter = 0;
  }
  else
  {
    update_counter++;
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(config::sd_power_pin, OUTPUT);
  digitalWrite(config::sd_power_pin, HIGH);
  delay(10);
  spi.begin(39, 13, 40);

  if (!SD.begin(config::sd_cs_pin, spi, 80000000))
  {
    Serial.println("no SD card");
  }
  else
  {
    Serial.println("SD card ok");
  }

  read_config();

  Wifi_Config wifi_config;
  clock_model.get_wifi_config(wifi_config);

  if (wifi_config.ssid.length() == 0)
  {
    Serial.println("Brak SSID, uruchamiam AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("EInkClock-AP", "inzynier_domu");
    state = State::AP;
  }
  else
  {
    Serial.println("SSID ustawione, lacze jako STA");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_config.ssid.c_str(), wifi_config.pass.c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    state = State::normal;
  }

  clock_controller.setup_clock();

  screen.setup_screen();

  calendar_view.setup_calendar_list();
  clock_view.setup_calendar_list();

  lv_timer_create(update_date, 60000, NULL);
  delay(1000);

  pinMode(config::alarm_enable_button_pin, INPUT);
  pinMode(config::btn_pin, INPUT_PULLDOWN);
  pinMode(config::led_pin, OUTPUT);
  digitalWrite(config::led_pin, LOW);

  audio.setup();

  httpServer.begin();
}

bool check_button()
{
  static bool last_state = false;
  bool current_state = digitalRead(config::btn_pin);
  if (current_state != last_state)
  {
    last_state = current_state;
    return true;
  }
  else
  {
    return false;
  }
}

unsigned long lastHaUpdate = 0;
void loop()
{
  if (state != State::AP)
  {
    lv_timer_handler();
  }
  delay(10);

  if (state == State::alarm)
  {
    audio.play_audio();
    if (check_button())
    {
      state = State::normal;
      digitalWrite(config::led_pin, LOW);
    }
  }
  else if (state == State::normal)
  {
    if (check_button())
    {
      digitalWrite(config::led_pin, HIGH);
      alarm_controller.toggle_alarm();
      alarm_controller.update_view();
      digitalWrite(config::led_pin, LOW);
    }
  }

  server.handleClient();
}
