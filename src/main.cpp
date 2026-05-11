/**
 * @file main.cpp
 * @brief Application entry point — initializes all subsystems and runs the main event loop.
 */

#include "RTClib.h"
#include "alarm_model.h"
#include "logger.h"
#include "alarm_view.h"
#include "alarm_trigger.h"
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

Screen screen;                                                                          ///< E-ink screen driver instance.
DateTime screen_reffresh_time(2000, 1, 1, 3, 0);                                       ///< Time of day at which a full screen clear is triggered (03:00).

SPIClass spi = SPIClass(HSPI);                                                          ///< HSPI bus instance used by the SD card.

Audio audio;                                                                            ///< Audio playback driver instance.
DynamicJsonDocument docs(19508);                                                        ///< Shared JSON document buffer.

Alarm_model alarm_model;                                                                ///< Data model for alarm state.
Alarm_view alarm_view(&screen);                                                         ///< View responsible for rendering alarm information.
Alarm_controller alarm_controller(&alarm_model, &alarm_view);                           ///< Controller managing alarm logic.

Calendar_model calendar_model;                                                          ///< Data model for calendar events.
Calendar_view calendar_view(&screen);                                                   ///< View responsible for rendering calendar events.
Calendar_controller calendar_controller(&calendar_model, &calendar_view, &alarm_controller); ///< Controller managing calendar fetching and display.

Clock_model clock_model;                                                                ///< Data model for time and Wi-Fi configuration.
Clock_view clock_view(&screen);                                                         ///< View responsible for rendering the clock.
Clock_controller clock_controller(&clock_view, &clock_model);                           ///< Controller managing clock synchronization and display.

Weather_model weather_model;                                                            ///< Data model for weather forecast.
WebServer server(80);                                                                   ///< HTTP server listening on port 80.
HttpServer httpServer(server, clock_model, weather_model, calendar_model, audio);       ///< Application-level HTTP/MQTT server wrapper.

Weather_view weather_view(&screen);                                                     ///< View responsible for rendering weather data.
Weather_controller weather_controller(&weather_model, &weather_view, &httpServer);      ///< Controller managing weather fetching and display.

State state;                                                                            ///< Current application state.

static bool btn_stable_state = false;         ///< Debounced (stable) button state.
static bool btn_raw_state = false;            ///< Raw (non-debounced) button reading.
static unsigned long btn_change_time = 0;     ///< Timestamp of the last raw button state change (ms).
static int update_counter = 10;               ///< Counter driving periodic data refresh ticks.

static uint8_t reset_press_count = 0;         ///< Number of button presses counted within the reset window.
static unsigned long reset_window_start = 0;  ///< Timestamp when the current reset press window started (ms).
static unsigned long boot_time = 0;           ///< Timestamp recorded at end of setup(), used for boot-time guards (ms).

TaskHandle_t audioTaskHandle = nullptr;       ///< Handle for the FreeRTOS audio playback task.
volatile bool startAlarmAudio = false;        ///< Flag set by the main task to start/stop audio playback on Core 1.

struct Check_adapter : Alarm_check {
  bool check(const DateTime& now) override { return alarm_controller.check_alarm(now); }
} alarm_check_adapter;                        ///< Bridges Alarm_controller into the Alarm_trigger interface.

struct Mqtt_adapter : Alarm_mqtt {
  void send_action() override { httpServer.send_mqtt_action(); }
} alarm_mqtt_adapter;                         ///< Bridges HttpServer MQTT call into the Alarm_trigger interface.

struct Audio_adapter : Alarm_audio {
  void start() override { audio.start(); }
  void stop()  override { audio.stop();  }
} alarm_audio_adapter;                        ///< Bridges Audio into the Alarm_trigger interface.

Alarm_trigger alarm_trigger(alarm_check_adapter, alarm_mqtt_adapter, alarm_audio_adapter, startAlarmAudio); ///< Orchestrates the full alarm trigger sequence.

/**
 * @brief FreeRTOS task that continuously plays alarm audio when the startAlarmAudio flag is set.
 * @param pvParameters Unused task parameter.
 */
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

/**
 * @brief Reads the JSON configuration file from the SD card and applies settings to all subsystem models.
 */
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

/**
 * @brief Generates a unique device ID string derived from the ESP32 MAC address.
 * @return Twelve-character hexadecimal string representing the device ID.
 */
String get_device_id()
{
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  char id[13];
  snprintf(id, sizeof(id), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(id);
}

/**
 * @brief Updates the clock view and triggers alarm state if the alarm time is reached.
 */
void update_clock()
{
  clock_controller.update_view();
  DateTime now;
  clock_controller.get_time(now);

  if (alarm_trigger.try_trigger(now, state == State::AP))
  {
    digitalWrite(config::led_pin, HIGH);
    state = State::alarm;
  }
}

/**
 * @brief LVGL timer callback that checks WiFi connectivity and triggers reconnection if lost.
 * @param timer Pointer to the LVGL timer that fired this callback.
 */
static void wifi_watchdog(lv_timer_t* timer)
{
  if (state == State::AP || state == State::welcome_screen)
  {
    return;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Logger::warn("WIFI", "Connection lost, reconnecting...");
    WiFi.reconnect();
  }
}

/**
 * @brief LVGL timer callback that updates the clock and periodically fetches weather and calendar data.
 * @param timer Pointer to the LVGL timer that fired this callback.
 */
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
    if (httpServer.is_weather_from_ha())
    {
      httpServer.get_ha_weather();
    }
    update_counter = 0;
  }
  else
  {
    update_counter++;
  }
}

/**
 * @brief Initializes serial port, SD card, logger, and logs the reset reason.
 */
static void init_hardware()
{
  Serial.begin(115200);
  delay(3000);  // DEBUG: wait for serial monitor
  Serial.println("=== SETUP START ===");

  pinMode(config::sd_power_pin, OUTPUT);
  digitalWrite(config::sd_power_pin, HIGH);
  delay(10);
  spi.begin(39, 13, 40);
  if (!SD.begin(config::sd_cs_pin, spi, 80000000))
    Serial.println("no SD card");
  else
    Serial.println("SD card ok");

  Logger::setup("/logs.txt", 50);

  esp_reset_reason_t reset_reason = esp_reset_reason();
  const char* reset_str = "UNKNOWN";
  switch (reset_reason)
  {
    case ESP_RST_POWERON:   reset_str = "POWERON";   break;
    case ESP_RST_SW:        reset_str = "SW_RESET";  break;
    case ESP_RST_PANIC:     reset_str = "PANIC";     break;
    case ESP_RST_INT_WDT:   reset_str = "INT_WDT";  break;
    case ESP_RST_TASK_WDT:  reset_str = "TASK_WDT"; break;
    case ESP_RST_WDT:       reset_str = "WDT";       break;
    case ESP_RST_BROWNOUT:  reset_str = "BROWNOUT";  break;
    case ESP_RST_SDIO:      reset_str = "SDIO";      break;
    default: break;
  }
  Logger::info("BOOT", String("Reset reason: ") + reset_str + " | version: " + config::version);
}

/**
 * @brief Starts the device as a Wi-Fi access point with a fixed SSID and password.
 */
static void start_ap_mode()
{
  WiFi.mode(WIFI_AP);
  delay(100);
  bool ap_ok = WiFi.softAP("EInkClock-AP", "inzynier_domu");
  WiFi.setSleep(false);
  Serial.print("AP started: ");
  Serial.println(ap_ok ? "YES" : "NO");
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  state = State::AP;
}

/**
 * @brief Connects to Wi-Fi in STA mode using stored credentials, or falls back to AP mode on timeout or missing SSID.
 */
static void init_wifi()
{
  Wifi_Config wifi_config;
  clock_model.get_wifi_config(wifi_config);

  if (wifi_config.ssid.length() == 0)
  {
    Serial.println("Brak SSID, uruchamiam AP");
    start_ap_mode();
    return;
  }

  Serial.println("SSID ustawione, lacze jako STA");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_config.ssid.c_str(), wifi_config.pass.c_str());
  unsigned long wifi_start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifi_start < 60000)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi connected");
    Serial.print("IP:");
    Serial.println(WiFi.localIP());
    state = State::welcome_screen;
    return;
  }

  Logger::warn("WIFI", "Connection timeout, falling back to AP mode");
  start_ap_mode();
}

/**
 * @brief Derives the device ID from the MAC address and propagates it to the HTTP server and calendar model.
 */
static void apply_device_id()
{
  String deviceId = get_device_id();
  httpServer.set_device_id(deviceId);

  google_api_config calendar_config;
  calendar_model.get_config(calendar_config);  // preserve ical_url from read_config()
  calendar_config.device_id = deviceId;
  calendar_model.set_config(calendar_config);

  Serial.print("Device ID: ");
  Serial.println(deviceId);
}

/**
 * @brief Initializes the clock, screen, calendar and clock views, and registers LVGL periodic timers.
 */
static void init_subsystems()
{
  clock_controller.setup_clock();
  screen.setup_screen();
  calendar_view.setup_calendar_list();
  clock_view.setup_calendar_list();

  lv_timer_create(update_date, 60000, NULL);
  lv_timer_create(wifi_watchdog, 120000, NULL);
  delay(1000);
}

/**
 * @brief Configures GPIO pins for the alarm button, main button, and LED; initialises debounce state variables.
 */
static void init_gpio()
{
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
  update_counter = 8;
}

/**
 * @brief Initializes the audio driver and launches the audio playback task on Core 1.
 */
static void init_audio()
{
  audio.setup();
  xTaskCreatePinnedToCore(audioTask, "audioTask", 4096, nullptr, 1, &audioTaskHandle, 1);
}

/**
 * @brief Registers the HA clock entity (when not in AP mode) and starts the HTTP server.
 */
static void init_http_server()
{
  if (state != State::AP)
    httpServer.entity_clock_setup();
  httpServer.begin();
}

/**
 * @brief Sets the initial Wi-Fi status label, flushes the LVGL frame, and turns on the boot LED.
 */
static void init_ui_status()
{
  if (state == State::welcome_screen)
  {
    String ip = "IP: " + WiFi.localIP().toString();
    lv_label_set_text(ui_labwifistatus, ip.c_str());
  }
  else if (state == State::AP)
  {
    lv_label_set_text(ui_labwifistatus, "Access point");
  }

  lv_timer_handler();  // flush pending lv_screen_load(ui_Screen2) before loop starts
  digitalWrite(config::led_pin, HIGH);
}

void setup()
{
  init_hardware();

  if (checkAndPerformUpdateFromSD())
    return;

  read_config();
  init_wifi();
  apply_device_id();
  init_subsystems();
  init_gpio();
  init_audio();
  init_http_server();
  init_ui_status();
}

/**
 * @brief Reads the button state with debounce filtering and reports a stable edge.
 * @return true when a debounced button edge is detected, false otherwise.
 */
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

/**
 * @brief Checks whether the button has been pressed enough times within the reset window to trigger a config reset.
 * @return true if the reset threshold was reached, false otherwise.
 */
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

/**
 * @brief Overwrites the configuration file on the SD card with an empty JSON object.
 */
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
      alarm_trigger.stop();
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
