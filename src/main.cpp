#include "RTClib.h"
#include "lvgl.h"
#include "ui/ui.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <HTTPClient.h>
#include <SD.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <time.h>

// #include <NTPClient.h>

const char* ssid = "ssid";
const char* password = "pass";

constexpr uint8_t sd_cs_pin = 10;

#define SCR_WIDTH 792
#define SCR_HEIGHT 272
#define LVBUF ((SCR_WIDTH * SCR_HEIGHT / 8) + 8)

RTC_DS1307 m_rtc; ///< DS1307 RTC
DateTime now;
DynamicJsonDocument docs(19508);

GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display(GxEPD2_579_GDEY0579T93(45, 46, 47, 48));

static uint8_t lvBuffer[2][LVBUF];

String openWeatherMapApiKey = "apikey";
float lat = 50.2945;
float lon = 18.6714;

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
void getWeather()
{
  String dateStr = getDateString(now);
  String serverPath = "https://api.openweathermap.org/data/3.0/onecall/day_summary?lat=" + String(lat, 4) + "&lon=" + String(lon, 4) +
                      "&date=" + dateStr + "&appid=" + openWeatherMapApiKey + "&units=metric";

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
  }
  else
  {
    Serial.print("Błąd HTTP: ");
    Serial.println(code);
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
    getWeather(); // Wywołaj tylko raz na 10 cykli
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

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Wire.begin(21, 38);

  if (!SD.begin(sd_cs_pin))
  {
    Serial.println("no SD card");
  }
  else
  {
    Serial.println("SD card ok");
  }

  if (!m_rtc.begin())
  {
    Serial.println("Couldn't find RTC");
  }
  m_rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

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
