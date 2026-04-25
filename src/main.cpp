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
#include "firmware_update.h"
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
  AP,
  welcome_screen
};

Screen screen;
DateTime screen_reffresh_time(2000, 1, 1, 3, 0);

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

static bool btn_stable_state = false;
static bool btn_raw_state = false;
static unsigned long btn_change_time = 0;
static int update_counter = 10;

static uint8_t reset_press_count = 0;
static unsigned long reset_window_start = 0;
static unsigned long boot_time = 0;

TaskHandle_t audioTaskHandle = nullptr;
volatile bool startAlarmAudio = false;

void audioTask(void* pvParameters)
{
  for (;;)
  {
    if (startAlarmAudio)
    {
      audio.play_audio();
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

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

  JsonDocument doc;
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
  Serial.printf("[CONFIG] SSID read: '%s'\n", wifi_config.ssid.c_str());

  Open_weather_config weather_config;
  weather_config.api_key = doc["openweathermap_api_key"] | "";
  weather_config.lat = doc["lat"];
  weather_config.lon = doc["lon"];
  weather_model.set_config(weather_config);

  HA_config ha_config;
  ha_config.ha_host = doc["HA_host"] | "";
  ha_config.ha_port = doc["HA_port"];
  ha_config.ha_token = doc["HA_token"] | "";
  ha_config.ha_user = doc["HA_user"] | "";
  ha_config.ha_pass = doc["HA_pass"] | "";
  ha_config.ha_enitty_weather_name = doc["HA_weather_entity_name"] | "";
  ha_config.ha_entity_clock_name = doc["HA_clock_entity_name"] | "";
  ha_config.mqtt_port = doc["mqtt_port"];
  ha_config.weather_from_ha = doc["weather_from_HA"];
  httpServer.ha_set_config(ha_config);

  google_api_config calendar_config;
  calendar_config.ical_url = doc["ical_url"] | "";
  calendar_config.ical_alarm_url = doc["ical_alarm_url"] | "";
  calendar_model.set_config(calendar_config);

  // Audio config is hard-coded (44100 Hz, 70% volume) — not configurable via web UI
  // Audio_config audio_config;
  // audio_config.sample_rate = doc["sample_rate"];
  // audio_config.volume = doc["volume"];
  // audio.set_config(audio_config);
}

String get_device_id()
{
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  char id[13];
  snprintf(id, sizeof(id), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(id);
}

void update_clock()
{
  clock_controller.update_view();
  DateTime now;
  clock_controller.get_time(now);

  if (alarm_controller.check_alarm(now) && state != State::alarm)
  {
    if (state != State::AP)
    {
      httpServer.send_mqtt_action();
      delay(200);
    }
    digitalWrite(config::led_pin, HIGH);
    state = State::alarm;
    audio.start();
    startAlarmAudio = true;
  }
}

static void update_date(lv_timer_t* timer)
{
  if (state == State::welcome_screen || state == State::AP)
  {
    return;
  }
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
  delay(3000);  // DEBUG: wait for serial monitor
  Serial.println("=== SETUP START ===");
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

  if (checkAndPerformUpdateFromSD())
  {
    return;
  }

  read_config();

  Wifi_Config wifi_config;
  clock_model.get_wifi_config(wifi_config);
  if (wifi_config.ssid.length() == 0)
  {
    Serial.println("Brak SSID, uruchamiam AP");
    WiFi.mode(WIFI_AP);
    delay(100);
    bool ap_ok = WiFi.softAP("EInkClock-AP", "inzynier_domu");
    Serial.print("AP started: ");
    Serial.println(ap_ok ? "YES" : "NO");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    state = State::AP;
  }
  else
  {
    Serial.println("SSID ustawione, lacze jako STA");
    WiFi.mode(WIFI_STA);
    // WiFi.setHostname("EInkClock");
    WiFi.begin(wifi_config.ssid.c_str(), wifi_config.pass.c_str());
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("WiFi connected");
    state = State::welcome_screen;
    Serial.print("IP:");
    Serial.println(WiFi.localIP());
  }

  // Generate and set device ID for pairing and calendar fetching
  String deviceId = get_device_id();
  httpServer.set_device_id(deviceId);

  google_api_config calendar_config;
  calendar_model.get_config(calendar_config);  // preserve ical_url from read_config()
  calendar_config.device_id = deviceId;
  calendar_model.set_config(calendar_config);

  Serial.print("Device ID: ");
  Serial.println(deviceId);

  clock_controller.setup_clock();
  screen.setup_screen();
  calendar_view.setup_calendar_list();
  clock_view.setup_calendar_list();

  if (state == State::welcome_screen)
  {
    String ip = "IP: " + WiFi.localIP().toString();
    lv_label_set_text(ui_labwifistatus, ip.c_str());
  }

  lv_timer_create(update_date, 60000, NULL);
  delay(1000);

  pinMode(config::alarm_enable_button_pin, INPUT);
  pinMode(config::btn_pin, INPUT_PULLDOWN);
  pinMode(config::led_pin, OUTPUT);
  digitalWrite(config::led_pin, LOW);
  btn_stable_state = digitalRead(config::btn_pin);
  btn_raw_state = btn_stable_state;
  btn_change_time = millis();
  reset_press_count = 0;
  reset_window_start = 0;
  boot_time = millis();
  update_counter = 10;

  audio.setup();
  xTaskCreatePinnedToCore(audioTask, "audioTask", 4096, nullptr, 1, &audioTaskHandle, 0);

  if (state != State::AP)
  {
    httpServer.entity_clock_setup();
  }
  httpServer.begin();

  lv_timer_handler();  // flush pending lv_screen_load(ui_Screen2) before loop starts

  digitalWrite(config::led_pin, HIGH);
}

bool check_button()
{
  bool reading = digitalRead(config::btn_pin);
  if (reading != btn_raw_state)
  {
    btn_raw_state = reading;
    btn_change_time = millis();
  }
  if (btn_raw_state != btn_stable_state && millis() - btn_change_time > config::btn_debounce_ms)
  {
    btn_stable_state = btn_raw_state;
    Serial.printf("[BTN] edge detected, stable=%d t=%lu\n", btn_stable_state, millis());
    return true;
  }
  return false;
}

bool check_reset_sequence()
{
  if (millis() - boot_time < 3000)
  {
    Serial.printf("[RST] blocked (boot protection), t=%lu boot=%lu\n", millis(), boot_time);
    return false;
  }

  unsigned long now = millis();
  if (reset_press_count == 0 || (now - reset_window_start > config::reset_window_ms))
  {
    reset_press_count = 1;
    reset_window_start = now;
    Serial.println("[RST] count=1 (new window)");
    return false;
  }
  reset_press_count++;
  Serial.printf("[RST] count=%d\n", reset_press_count);
  if (reset_press_count >= config::reset_press_count_threshold)
  {
    reset_press_count = 0;
    Serial.println("[RST] TRIGGERED - clearing config!");
    return true;
  }
  return false;
}

void clear_config()
{
  File file = SD.open(config::config_path, "w");
  if (file)
  {
    file.print("{}");
    file.close();
    Serial.println("Config cleared");
  }
  else
  {
    Serial.println("Failed to clear config file");
  }
}

void loop()
{
  bool btn_edge = check_button();

  lv_timer_handler();
  delay(10);
  if (state == State::alarm)
  {
    if (btn_edge)
    {
      state = State::normal;
      audio.stop();
      startAlarmAudio = false;
      digitalWrite(config::led_pin, LOW);
    }
  }
  else if (state == State::welcome_screen)
  {
    if (btn_edge)
    {
      if (check_reset_sequence())
      {
        clear_config();
        ESP.restart();
      }
      else
      {
        digitalWrite(config::led_pin, LOW);
        lv_scr_load(ui_Screen1);
        state = State::normal;
      }
    }
  }
  else if (state == State::normal)
  {
    if (btn_edge)
    {
      if (check_reset_sequence())
      {
        clear_config();
        ESP.restart();
      }
      else
      {
        digitalWrite(config::led_pin, HIGH);
        alarm_controller.toggle_alarm();
        alarm_controller.update_view();
        digitalWrite(config::led_pin, LOW);
      }
    }
  }

  if (clock_controller.is_it_now(screen_reffresh_time))
  {
    screen.full_clear();
  }
  server.handleClient();
}
