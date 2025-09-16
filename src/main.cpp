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
#include <time.h>

struct Simple_time
{
  uint8_t hour;
  uint8_t minutes;
};
struct Calendar_event
{
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

SPIClass spi = SPIClass(HSPI);

Open_weather_config weather_config;
String google_script;

constexpr uint8_t sd_cs_pin = 10;

#define SCR_WIDTH 792
#define SCR_HEIGHT 272
#define LVBUF ((SCR_WIDTH * SCR_HEIGHT / 8) + 8)

RTC_DS1307 m_rtc; ///< DS1307 RTC
DateTime now;
DynamicJsonDocument docs(19508);

GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display(GxEPD2_579_GDEY0579T93(45, 46, 47, 48));

static uint8_t lvBuffer[2][LVBUF];

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
  google_script = doc["google_script"] | "";
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

void update_clock()
{
  now = m_rtc.now();
  char buf[4];
  sprintf(buf, "%02d:%02d", now.hour(), now.minute());
  lv_label_set_text(ui_labtime, buf);

  char buf_date[11]; // "rrrr-mm-dd" + null
  sprintf(buf_date, "%04d-%02d-%02d", now.year(), now.month(), now.day());
  lv_label_set_text(ui_labData, buf_date);
}

String getDateString(DateTime dt)
{
  char dateStr[11];
  sprintf(dateStr, "%04d-%02d-%02d", dt.year(), dt.month(), dt.day());
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

char weather_icon_change(int cloud_cover, int precipitation)
{
  if (cloud_cover > 30 && precipitation == 0)
  {
    return 'ú';
  }

  if (precipitation < 5 && precipitation != 0)
  {
    return 'û';
  }

  if (precipitation > 5)
  {
    return 'ü';
  }

  return 'ù';
}

void getWeather()
{
  String dateStr = getDateString(now);
  Serial.println(weather_config.api_key);
  String serverPath = "https://api.openweathermap.org/data/3.0/onecall/day_summary?lat=" + String(weather_config.lat, 4) +
                      "&lon=" + String(weather_config.lon, 4) + "&date=" + dateStr + "&appid=" + weather_config.api_key + "&units=metric";

  HTTPClient http;
  http.begin(serverPath.c_str());
  int code = http.GET();

  if (code == HTTP_CODE_OK)
  {
    String response = http.getString();

    // Parsowanie zakomentowane - odkomentuj, gdy JSON będzie poprawny
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

    int temp_rounded = (int)round(temp);
    char temp_str[10];
    sprintf(temp_str, "%d℃", temp_rounded);

    lv_label_set_text(ui_labTempMorning, temp_str);
    int precipitation = docs["precipitation"]["total"];
    Serial.println(precipitation);
    int cloud_cover = docs["cloud_cover"]["afternoon"];
    Serial.println(cloud_cover);
    char weather_icon = weather_icon_change(cloud_cover, precipitation);
    char icon[1];
    icon[0] = weather_icon;
    lv_label_set_text(ui_Label4, icon);
  }
  else
  {
    Serial.print("Błąd HTTP: ");
    Serial.println(code);
  }
  http.end();
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

void get_calendar()
{
  HTTPClient http;
  http.setTimeout(20000);
  String url = "https://script.google.com/macros/s/" + google_script + "/exec";
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
    Serial.println(response);

    DynamicJsonDocument doc(8192); // Zwiększ jeśli potrzebujesz
    DeserializationError error = deserializeJson(doc, response);

    if (!error && doc["success"])
    {
      JsonArray events = doc["events"];
      for (JsonObject event : events)
      {
        Serial.print("Tytuł: ");
        Serial.println(event["title"].as<String>());
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
  if (weatherCounter >= 5)
  {
    if (internetWorks())
    {
      Serial.println("wifi ok");
    }
    getWeather(); // Wywołaj tylko raz na 10 cykli
    get_calendar();
    weatherCounter = 0; // Resetuj licznik
  }
  else
  {
    weatherCounter++; // Zwiększ licznik
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(42, OUTPUT);
  digitalWrite(42, HIGH);
  delay(10);
  spi.begin(39, 13, 40);

  if (!SD.begin(sd_cs_pin, spi, 80000000))
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

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  epd_setup();

  lv_init();
  lv_tick_set_cb(my_tick);

  lv_display_t* disp = lv_display_create(SCR_WIDTH, SCR_HEIGHT);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, lvBuffer[0], lvBuffer[1], LVBUF, LV_DISPLAY_RENDER_MODE_PARTIAL);

  ui_init();

  lv_obj_set_style_text_color(ui_Screen1, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_timer_create(update_date, 60000, NULL);
  delay(1000);
}

void loop()
{
  lv_timer_handler(); // powinien być wywoływany bardzo często (np. co 10ms)
  delay(10); // minimalny delay dla stability
}
