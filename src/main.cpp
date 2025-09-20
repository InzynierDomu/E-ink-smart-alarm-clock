#include "RTClib.h"
#include "config.h"
#include "lvgl.h"
#include "ui/ui.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <HTTPClient.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <driver/i2s.h>
#include <time.h>
#include <vector>

struct Simple_time
{
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

struct Clock_alarm
{
  Clock_alarm()
  : time{Simple_time(0, 0)}
  , enable(false)
  {}
  Simple_time time;
  bool enable;
};

struct Simple_weather
{
  uint8_t temperature_morning;
  uint8_t temperature_afternoon;
  uint8_t temperature_evening;
  uint8_t cloud_cover;
  uint8_t precipitation;
};

struct Calendar_event
{
  Calendar_event(String& _name, String& _calendar, Simple_time& _time_start, Simple_time& _time_stop)
  : name(_name)
  , calendar(_calendar)
  , time_start(_time_start)
  , time_stop(_time_stop)
  {}
  String get_calendar_label()
  {
    String label = time_start.to_string() + " " + name;
    return label;
  }
  String name;
  String calendar;
  Simple_time time_start;
  Simple_time time_stop;
};

struct Wifi_Config
{
  String ssid;
  String pass;
  long timezone;
};

struct Open_weather_config
{
  String api_key;
  float lat;
  float lon;
};

struct google_api_config
{
  String script_url;
  String alarm_calendar_id;
  String google_calendar_id;
};

SPIClass spi = SPIClass(HSPI);

Open_weather_config weather_config;

std::vector<lv_obj_t*> calendar_labels;

google_api_config google_config;

std::vector<Calendar_event> calendar;
std::vector<Simple_weather> forecast;
Clock_alarm clock_alarm;

#define SCR_WIDTH 792
#define SCR_HEIGHT 272
#define LVBUF ((SCR_WIDTH * SCR_HEIGHT / 8) + 8)

constexpr uint8_t speaker_bck_pin = 19;
constexpr uint8_t speaker_ws_pin = 20;
constexpr uint8_t speaker_dout_pin = 3;
const uint16_t buffer_size = 512;
uint16_t sample_rate = 16000;

RTC_DS1307 m_rtc; ///< DS1307 RTC
DateTime now;
DynamicJsonDocument docs(19508);

GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display(GxEPD2_579_GDEY0579T93(45, 46, 47, 48));

static uint8_t lvBuffer[2][LVBUF];

void playAudio()
{
  File audioFile;
  Serial.println("playing audio...");
  String file_path = "/ringtone.wav";
  audioFile = SD.open(file_path, FILE_READ);
  if (!audioFile)
  {
    Serial.println("error with audio file");
    return;
  }

  i2s_start(I2S_NUM_1);
  size_t bytesRead;
  int16_t buffer[buffer_size];

  while (audioFile.available())
  {
    bytesRead = audioFile.read((uint8_t*)buffer, buffer_size * sizeof(int16_t));
    i2s_write(I2S_NUM_1, buffer, bytesRead, &bytesRead, portMAX_DELAY);
  }

  audioFile.close();

  i2s_stop(I2S_NUM_1);

  Serial.println("playing end.");
}

void read_config(Wifi_Config& _config)
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

  StaticJsonDocument<512> doc;

  DeserializationError error = deserializeJson(doc, jsonData);
  if (error)
  {
    Serial.print("Błąd parsowania JSON: ");
    Serial.println(error.c_str());
    return;
  }

  _config.ssid = doc["ssid"] | "";
  _config.pass = doc["pass"] | "";
  _config.timezone = doc["timezone"];
  Serial.println(doc["api_key"] | "");
  weather_config.api_key = doc["api_key"] | "";
  weather_config.lat = doc["lat"];
  weather_config.lon = doc["lon"];
  google_config.script_url = doc["google_script_url"] | "";
  google_config.alarm_calendar_id = doc["google_calendar_alarm_id"] | "";
  google_config.google_calendar_id = doc["google_calendar_id"] | "";
}

void my_disp_flush(lv_display_t* disp, const lv_area_t* area, unsigned char* data)
{
  int16_t width = area->x2 - area->x1 + 1;
  int16_t height = area->y2 - area->y1 + 1;
  display.drawImage((uint8_t*)data + 8, area->x1, area->y1, width, height);

  lv_display_flush_ready(disp);
}

static uint32_t my_tick(void)
{
  return millis();
}

void epd_setup()
{
  SPI.begin(12, -1, 11, 45);
  display.init(115200, true, 2, false);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
  delay(1000);
}

void check_alarm(DateTime& now)
{
  Serial.print("chek alarm");
  Serial.println(now.hour());
  Serial.println(clock_alarm.time.hour);
  if (now.hour() == clock_alarm.time.hour && now.minute() == clock_alarm.time.minutes)
  {
    Serial.println("ALARM!");
    playAudio();
  }
}

void update_clock()
{
  now = m_rtc.now();
  char buf[4];
  sprintf(buf, "%02d:%02d", now.hour(), now.minute());
  lv_label_set_text(ui_labtime, buf);

  char buf_date[11]; // "dd-mm" + null
  sprintf(buf_date, "%02d-%02d", now.day(), now.month());
  lv_label_set_text(ui_labDate, buf_date);
  sprintf(buf_date, "%02d-%02d", now.day() + 1, now.month());
  lv_label_set_text(ui_labDateDay1, buf_date);
  sprintf(buf_date, "%02d-%02d", now.day() + 2, now.month());
  lv_label_set_text(ui_labDateDay2, buf_date);
  sprintf(buf_date, "%02d-%02d", now.day() + 3, now.month());
  lv_label_set_text(ui_labDateDay3, buf_date);

  if (clock_alarm.enable)
  {
    check_alarm(now);
  }
}

String getDateString(DateTime dt, uint8_t offset = 0)
{
  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", dt.year(), dt.month(), (dt.day() + offset));
  return String(dateStr);
}

enum class wheater
{
  sunny,
  claudy,
  shower,
  rain
};
// todo change to map

const char* weather_icon_change(int cloud_cover, int precipitation)
{
  if (cloud_cover > 30 && precipitation == 0)
    return "ú";

  if (precipitation < 5 && precipitation != 0)
    return "û";

  if (precipitation > 5)
    return "ü";

  return "ù";
}

void print_weather()
{
  char temp_str[10];
  sprintf(temp_str, "%d℃", forecast[0].temperature_afternoon);
  lv_label_set_text(ui_labTempMorning, temp_str);
  const char* icon = weather_icon_change(forecast[0].cloud_cover, forecast[0].precipitation);
  lv_label_set_text(ui_labWeatherIcon, icon);

  sprintf(temp_str, "%d℃", forecast[1].temperature_morning);
  lv_label_set_text(ui_labTempMorningDay1, temp_str);
  sprintf(temp_str, "%d℃", forecast[1].temperature_afternoon);
  lv_label_set_text(ui_labTempAfternoonDay1, temp_str);
  sprintf(temp_str, "%d℃", forecast[1].temperature_evening);
  lv_label_set_text(ui_labTempEveningDay1, temp_str);
  const char* icon1 = weather_icon_change(forecast[1].cloud_cover, forecast[1].precipitation);
  lv_label_set_text(ui_labWeatherIconDay1, icon1);

  sprintf(temp_str, "%d℃", forecast[2].temperature_morning);
  lv_label_set_text(ui_labTempMorningDay2, temp_str);
  sprintf(temp_str, "%d℃", forecast[2].temperature_afternoon);
  lv_label_set_text(ui_labTempAfternoonDay2, temp_str);
  sprintf(temp_str, "%d℃", forecast[2].temperature_evening);
  lv_label_set_text(ui_labTempEveningDay2, temp_str);
  const char* icon2 = weather_icon_change(forecast[2].cloud_cover, forecast[2].precipitation);
  lv_label_set_text(ui_labWeatherIconDay2, icon2);

  sprintf(temp_str, "%d℃", forecast[3].temperature_morning);
  lv_label_set_text(ui_labTempMorningDay3, temp_str);
  sprintf(temp_str, "%d℃", forecast[3].temperature_afternoon);
  lv_label_set_text(ui_labTempAfternoonDay3, temp_str);
  sprintf(temp_str, "%d℃", forecast[3].temperature_evening);
  lv_label_set_text(ui_labTempEveningDay3, temp_str);
  const char* icon3 = weather_icon_change(forecast[3].cloud_cover, forecast[3].precipitation);
  lv_label_set_text(ui_labWeatherIconDay3, icon3);
}

void getWeather()
{
  forecast.clear();
  for (size_t i = 0; i < 4; i++)
  {
    String dateStr = getDateString(now, i);
    Serial.println(weather_config.api_key);
    String serverPath = "https://api.openweathermap.org/data/3.0/onecall/day_summary?lat=" + String(weather_config.lat, 4) +
                        "&lon=" + String(weather_config.lon, 4) + "&date=" + dateStr + "&appid=" + weather_config.api_key + "&units=metric";

    HTTPClient http;
    http.begin(serverPath.c_str());
    int code = http.GET();

    if (code == HTTP_CODE_OK)
    {
      String response = http.getString();

      StaticJsonDocument<4096> docs;
      DeserializationError error = deserializeJson(docs, response);

      if (error)
      {
        Serial.print("Błąd JSON: ");
        Serial.println(error.c_str());
        http.end();
        return;
      }

      float temp = docs["temperature"]["afternoon"]; // Dostosuj gdy JSON poprawny

      Serial.print("Data: ");
      Serial.print(dateStr);
      Serial.print(" | Temp: ");
      Serial.println(temp);

      Simple_weather forecast_weather;

      forecast_weather.temperature_afternoon = (int)round(temp);
      temp = docs["temperature"]["morning"];
      forecast_weather.temperature_morning = (int)round(temp);
      temp = docs["temperature"]["evening"];
      forecast_weather.temperature_evening = (int)round(temp);
      forecast_weather.precipitation = docs["precipitation"]["total"];
      forecast_weather.cloud_cover = docs["cloud_cover"]["afternoon"];
      forecast.push_back(forecast_weather);
    }
    else
    {
      Serial.print("Błąd HTTP: ");
      Serial.println(code);
    }
    http.end();
  }
}

bool internetWorks()
{
  HTTPClient http;
  if (http.begin("script.google.com", 443))
  {
    http.end();
    return true;
  }
  else
  {
    http.end();
    return false;
  }
}

void print_calendar()
{
  size_t n = calendar_labels.size();
  size_t m = calendar.size();
  if (m > n)
  {
    m = n;
  }
  Serial.print("wektory:");
  Serial.print(m);
  Serial.print(",");
  Serial.println(n);

  for (size_t i = 0; i < n; ++i)
  {
    if (i < m)
    {
      String label = calendar[i].get_calendar_label();
      Serial.println(label);
      lv_label_set_text(calendar_labels[i], label.c_str());
    }
    else
    {
      lv_label_set_text(calendar_labels[i], "-");
    }
  }
}

void get_calendar()
{
  HTTPClient http;
  http.setTimeout(20000);
  String url = "https://script.google.com/macros/s/" + google_config.script_url + "/exec";
  if (!http.begin(url))
  {
    Serial.println("Cannot connect to google script");
  }

  int httpResponseCode = http.GET();

  if (httpResponseCode == 301 || httpResponseCode == 302)
  {
    String newUrl = http.getLocation(); // Pobierz nowy URL z nagłówka Location
    Serial.printf("Przekierowanie: %d -> %s\n", httpResponseCode, newUrl.c_str());
    http.end();

    // Wywołaj ponownie do nowego URL (uwaga: rekurencja lub pętla, ale ogranicz max ilość przekierowań)
    http.begin(newUrl);
    httpResponseCode = http.GET();
  }

  if (httpResponseCode == 200)
  {
    String response = http.getString();
    Serial.println("JSON received:");

    DynamicJsonDocument doc(8192); // Zwiększ jeśli potrzebujesz
    DeserializationError error = deserializeJson(doc, response);
    bool is_alarm = false;
    if (!error && doc["success"])
    {
      calendar.clear();
      JsonArray events = doc["events"];
      for (JsonObject event : events)
      {
        String name = event["title"] | "";
        String calendar_name = event["calendarName"] | "";
        String temp_time = event["startTime"] | "";
        Simple_time start(temp_time);
        temp_time = event["endTime"] | "";
        Simple_time end(temp_time);
        Calendar_event new_event(name, calendar_name, start, end);
        Serial.println(new_event.name);
        Serial.println(new_event.calendar);
        if (calendar_name == google_config.alarm_calendar_id)
        {
          clock_alarm.time = new_event.time_start;
          lv_label_set_text(ui_labAlarm, clock_alarm.time.to_string().c_str());
          is_alarm = true;
        }
        else if (calendar_name == google_config.google_calendar_id)
        {
          calendar.push_back(new_event);
        }
      }
      print_calendar();
      if (!is_alarm)
      {
        lv_label_set_text(ui_labAlarmEnable, "OFF");
      }
    }
    else
    {
      Serial.println("JSON parse error lub brak wydarzeń");
    }
  }
  else
  {
    Serial.printf("HTTP error: %d\n", httpResponseCode);
  }
  http.end();
}

static void update_date(lv_timer_t* timer)
{
  static int weatherCounter = 0;
  update_clock();
  Serial.println(weatherCounter);
  if (weatherCounter >= 10)
  {
    if (internetWorks())
    {
      Serial.println("wifi ok");
    }
    getWeather(); // Wywołaj tylko raz na 10 cykli
    print_weather();
    get_calendar();
    weatherCounter = 0; // Resetuj licznik
  }
  else
  {
    weatherCounter++; // Zwiększ licznik
  }
}

void setupI2SSpeaker()
{
  Serial.println("audio cofing start");
  i2s_config_t i2s_config = {.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
                             .sample_rate = sample_rate,
                             .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                             .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
                             .communication_format = I2S_COMM_FORMAT_I2S,
                             .intr_alloc_flags = 0,
                             .dma_buf_count = 8,
                             .dma_buf_len = buffer_size,
                             .use_apll = false};
  i2s_pin_config_t pin_config = {
      .bck_io_num = speaker_bck_pin, .ws_io_num = speaker_ws_pin, .data_out_num = speaker_dout_pin, .data_in_num = I2S_PIN_NO_CHANGE};
  esp_err_t err = i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
  Serial.println(esp_err_to_name(err));
  err = i2s_set_pin(I2S_NUM_1, &pin_config);
  Serial.println(esp_err_to_name(err));
  Serial.println("audio cofing end");
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

  Wifi_Config wifi_config;
  read_config(wifi_config);

  WiFi.begin(wifi_config.ssid, wifi_config.pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  configTime(wifi_config.timezone, 3600, config::time_server);

  tm time_info;
  if (!getLocalTime(&time_info))
  {
    Serial.println("Błąd pobierania czasu!");
  }
  else
  {
    Serial.println("Czas pobrany z NTP:");
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                  time_info.tm_year + 1900,
                  time_info.tm_mon + 1,
                  time_info.tm_mday,
                  time_info.tm_hour,
                  time_info.tm_min,
                  time_info.tm_sec);
  }

  Wire.begin(21, 38);

  if (!m_rtc.begin())
  {
    Serial.println("Couldn't find RTC");
  }
  DateTime dt(time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
  m_rtc.adjust(dt);

  pinMode(config::screen_power_pin, OUTPUT);
  digitalWrite(config::screen_power_pin, HIGH);

  epd_setup();

  lv_init();
  lv_tick_set_cb(my_tick);

  lv_display_t* disp = lv_display_create(SCR_WIDTH, SCR_HEIGHT);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, lvBuffer[0], lvBuffer[1], LVBUF, LV_DISPLAY_RENDER_MODE_PARTIAL);

  ui_init();

  lv_obj_set_style_text_color(ui_Screen1, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN | LV_STATE_DEFAULT);

  calendar_labels.push_back(ui_labCalendarEvent1);
  calendar_labels.push_back(ui_labCalendarEvent2);
  calendar_labels.push_back(ui_labCalendarEvent3);
  calendar_labels.push_back(ui_labCalendarEvent4);
  calendar_labels.push_back(ui_labCalendarEvent5);
  calendar_labels.push_back(ui_labCalendarEvent6);
  calendar_labels.push_back(ui_labCalendarEvent7);

  lv_timer_create(update_date, 60000, NULL);
  delay(1000);

  pinMode(config::alarm_enable_button_pin, INPUT);
}

void loop()
{
  lv_timer_handler(); // powinien być wywoływany bardzo często (np. co 10ms)
  delay(10); // minimalny delay dla stability
  if (digitalRead(config::alarm_enable_button_pin) == LOW)
  {
    clock_alarm.enable = !clock_alarm.enable;
    if (clock_alarm.enable)
    {
      lv_label_set_text(ui_labAlarmEnable, "ON");
    }
    else
    {
      lv_label_set_text(ui_labAlarmEnable, "OFF");
    }
  }
}
