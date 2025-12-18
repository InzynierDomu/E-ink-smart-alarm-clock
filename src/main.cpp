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

Weather_model weather_model;
Weather_view weather_view(&screen);
Weather_controller weather_controller(&weather_model, &weather_view);

Alarm_model alarm_model;
Alarm_view alarm_view(&screen);
Alarm_controller alarm_controller(&alarm_model, &alarm_view);

Calendar_model calendar_model;
Calendar_view calendar_view(&screen);
Calendar_controller calendar_controller(&calendar_model, &calendar_view, &alarm_controller);

Clock_model clock_model;
Clock_view clock_view(&screen);
Clock_controller clock_controller(&clock_view, &clock_model);

WebServer server(80);

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
  weather_config.api_key = doc["api_key"] | "";
  weather_config.lat = doc["lat"];
  weather_config.lon = doc["lon"];
  weather_model.set_config(weather_config);

  google_api_config google_config;
  google_config.script_url = doc["google_script_url"] | "";
  google_config.alarm_calendar_id = doc["google_calendar_alarm_id"] | "";
  google_config.google_calendar_id = doc["google_calendar_id"] | "";
  calendar_model.set_config(google_config);

  uint16_t sample_rate = doc["sample_rate"];
  audio.set_sample_rate(sample_rate);
  uint8_t volume = doc["volume"];
  audio.set_volume(volume);
}

String getPage()
{
  String page = "<!DOCTYPE html>"
                "<html><head><meta charset='UTF-8'><title>Konfiguracja</title>"
                "<style>body{font-family:sans-serif;}.row{margin:4px 0;}"
                ".name{display:inline-block;width:220px;}input{width:260px;}</style>"
                "</head><body><h1>Konfiguracja</h1>";

  Wifi_Config wifi_config;
  clock_model.get_wifi_config(wifi_config);

  Open_weather_config weather_config;
  weather_model.get_config(weather_config); // jeśli nie masz, dorób getter

  page += "<form method='POST' action='/save'>";

  page += "<div class='row'><span class='name'>ssid</span>"
          "<input type='text' name='ssid' value='" +
          String(wifi_config.ssid) + "'></div>";

  page += "<div class='row'><span class='name'>lat</span>"
          "<input type='text' name='lat' value='" +
          String(weather_config.lat) + "'></div>";

  page += "<div class='row'><button type='submit'>Zapisz i zrestartuj</button></div>";

  page += "</form></body></html>";

  return page;
}
void handleRoot()
{
  server.send(200, "text/html", getPage());
}

void handleSave()
{
  if (!server.hasArg("ssid") || !server.hasArg("lat"))
  {
    server.send(400, "text/plain", "Brak pola ssid lub lat");
    return;
  }

  String new_ssid = server.arg("ssid");
  String new_lat_str = server.arg("lat");
  float new_lat = new_lat_str.toFloat(); // konwersja na float

  File file = SD.open(config::config_path, "r");
  if (!file)
  {
    server.send(500, "text/plain", "Brak pliku config");
    return;
  }

  String jsonData;
  while (file.available())
  {
    jsonData += (char)file.read();
  }
  file.close();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonData);
  if (error)
  {
    server.send(500, "text/plain", "Blad parsowania JSON");
    return;
  }

  // aktualizacja w JSON
  doc["ssid"] = new_ssid;
  doc["lat"] = new_lat; // w pliku będzie liczba

  file = SD.open(config::config_path, "w");
  if (!file)
  {
    server.send(500, "text/plain", "Nie moge otworzyc config do zapisu");
    return;
  }
  if (serializeJson(doc, file) == 0)
  {
    file.close();
    server.send(500, "text/plain", "Blad zapisu JSON");
    return;
  }
  file.close();

  server.send(200, "text/plain", "Zapisano, restartuje...");
  delay(500);
  ESP.restart();
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
    WiFi.softAP("EInkClock-AP", "12345678");
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

  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
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
