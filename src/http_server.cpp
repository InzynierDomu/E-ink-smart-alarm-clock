#include "http_server.h"

#include "config.h"

#include <ArduinoJson.h>
#include <SD.h>


HttpServer::HttpServer(WebServer& server, Clock_model& clock_model, Weather_model& weather_model, Calendar_model& calendar_model,
                       Audio& audio)
: server_(server)
, clock_model_(clock_model)
, weather_model_(weather_model)
, calendar_model_(calendar_model)
, audio_(audio)
{}

void HttpServer::begin()
{
  server_.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server_.on("/save", HTTP_POST, [this]() { this->handleSave(); });
  server_.begin();
}

void HttpServer::ha_set_config(HA_config& config)
{
  ha_config = config;
}

void HttpServer::get_ha_weather()
{
  String haTemperature = "--";
  Serial.println("=== updateHaMeasurement ===");

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[HA] WiFi not connected");
    return;
  }

  WiFiClient client;
  Serial.printf("[HA] Connecting to %s:%u\n", ha_config.ha_host, ha_config.ha_port);
  if (!client.connect(ha_config.ha_host.c_str(), ha_config.ha_port))
  {
    Serial.println("[HA] Connection failed");
    return;
  }
  Serial.println("[HA] Connected, sending request");

  String url = "/api/states/" + String(ha_config.ha_enitty_weather_name);
  String request = String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + String(ha_config.ha_host) + "\r\n" + "Authorization: Bearer " +
                   String(ha_config.ha_token) + "\r\n" + "Connection: close\r\n\r\n";

  Serial.println("[HA] Request:");
  Serial.println(request);

  client.print(request);

  // Odbiór nagłówków
  String headers;
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (line == "\r")
    {
      break; // koniec nagłówków
    }
    headers += line + "\n";
  }
  Serial.println("[HA] Headers:");
  Serial.println(headers);

  // Prosta walidacja kodu odpowiedzi
  if (!headers.startsWith("HTTP/1.1 200"))
  {
    Serial.println("[HA] Non-200 response");
    return;
  }

  // Odbiór body
  String body;
  while (client.available())
  {
    body += client.readString();
  }
  client.stop();

  Serial.println("[HA] Body:");
  Serial.println(body);

  // Parsowanie JSON
  int idx = body.indexOf("\"state\":");
  if (idx < 0)
  {
    Serial.println("[HA] 'state' not found in JSON");
    return;
  }

  int start = body.indexOf("\"", idx + 8);
  int end = body.indexOf("\"", start + 1);
  if (start < 0 || end < 0 || end <= start)
  {
    Serial.println("[HA] Failed to parse state string");
    return;
  }

  haTemperature = body.substring(start + 1, end);
  Serial.print("[HA] Parsed state: ");
  Serial.println(haTemperature);
}

String HttpServer::buildWifiSection()
{
  Wifi_Config wifi;
  clock_model_.get_wifi_config(wifi);

  String html;
  html += "<h2>WiFi</h2>";
  html += "<div class='row'><span class='name'>SSID</span>"
          "<input type='text' name='ssid' value='" +
          String(wifi.ssid) + "'></div>";
  html += "<div class='row'><span class='name'>Hasło</span>"
          "<input type='password' name='pass' value='" +
          String(wifi.pass) + "'></div>";
  return html;
}

String HttpServer::buildTimezoneSection()
{
  Wifi_Config wifi;
  clock_model_.get_wifi_config(wifi);

  // zakładamy, że w configu jest w sekundach
  int timezone_hours = wifi.timezone / 3600;

  String html;
  html += "<h2>Strefa czasowa</h2>";
  html += "<div class='row'><span class='name'>UTC offset [h]</span>"
          "<input type='number' step='1' name='timezone_hours' value='" +
          String(timezone_hours) + "'></div>";
  return html;
}

String HttpServer::buildWeatherSection()
{
  Open_weather_config weather;
  weather_model_.get_config(weather);

  String html;
  html += "<h2>OpenWeather API</h2>";
  html += "<div class='row'><span class='name'>API key</span>"
          "<input type='text' name='api_key' value='" +
          String(weather.api_key) + "'></div>";
  html += "<div class='row'><span class='name'>Szerokość (lat)</span>"
          "<input type='text' name='lat' value='" +
          String(weather.lat) + "'></div>";
  html += "<div class='row'><span class='name'>Długość (lon)</span>"
          "<input type='text' name='lon' value='" +
          String(weather.lon) + "'></div>";
  return html;
}

String HttpServer::buildAudioSection()
{
  uint16_t current_sr = audio_.get_sample_rate();
  uint8_t current_vol = audio_.get_volume();

  const uint16_t sr_list[] = {8000, 16000, 22050, 32000, 44100, 48000};

  String html;
  html += "<h2>Dźwięk</h2>";

  // Sample rate – select z typowymi wartościami
  html += "<div class='row'><span class='name'>Sample rate</span>"
          "<select name='sample_rate'>";
  for (uint8_t i = 0; i < sizeof(sr_list) / sizeof(sr_list[0]); i++)
  {
    html += "<option value='" + String(sr_list[i]) + "'";
    if (sr_list[i] == current_sr)
      html += " selected";
    html += ">" + String(sr_list[i]) + " Hz</option>";
  }
  html += "</select></div>";

  // Volume – slider 0–255
  html += "<div class='row'><span class='name'>Głośność</span>"
          "<input type='range' name='volume' min='0' max='255' value='" +
          String(current_vol) + "'></div>";

  return html;
}

String HttpServer::buildPage()
{
  String page = "<!DOCTYPE html>"
                "<html><head><meta charset='UTF-8'><title>Konfiguracja</title>"
                "<style>"
                "body{font-family:sans-serif;}"
                ".row{margin:4px 0;}"
                ".name{display:inline-block;width:220px;}"
                "input,select{width:260px;}"
                "h2{margin-top:16px;}"
                "</style>"
                "</head><body><h1>Konfiguracja</h1>";

  page += "<form method='POST' action='/save'>";
  page += buildWifiSection();
  page += buildTimezoneSection();
  page += buildWeatherSection();
  page += buildAudioSection();
  page += "<div class='row'><button type='submit'>Zapisz i zrestartuj</button></div>";
  page += "</form></body></html>";
  return page;
}

void HttpServer::handleRoot()
{
  server_.send(200, "text/html", buildPage());
}

void HttpServer::handleSave()
{
  StaticJsonDocument<1024> doc;
  if (!loadConfigJson(doc))
  {
    server_.send(500, "text/plain", "Blad odczytu config");
    return;
  }

  updateConfigFromRequest(doc);

  if (!saveConfigJson(doc))
  {
    server_.send(500, "text/plain", "Blad zapisu config");
    return;
  }

  server_.send(200, "text/plain", "Zapisano, restartuje...");
  delay(500);
  ESP.restart();
}

bool HttpServer::loadConfigJson(StaticJsonDocument<1024>& doc)
{
  File file = SD.open(config::config_path, "r");
  if (!file)
    return false;

  String jsonData;
  while (file.available())
    jsonData += (char)file.read();
  file.close();

  DeserializationError err = deserializeJson(doc, jsonData);
  return !err;
}

bool HttpServer::saveConfigJson(const StaticJsonDocument<1024>& doc)
{
  File file = SD.open(config::config_path, "w");
  if (!file)
    return false;

  bool ok = serializeJson(doc, file) > 0;
  file.close();
  return ok;
}

void HttpServer::updateConfigFromRequest(StaticJsonDocument<1024>& doc)
{
  String new_ssid = server_.arg("ssid");
  String new_pass = server_.arg("pass");
  int tz_hours = server_.arg("timezone_hours").toInt();
  int tz_seconds = tz_hours * 3600;

  String new_api_key = server_.arg("api_key");
  float new_lat = server_.arg("lat").toFloat();
  float new_lon = server_.arg("lon").toFloat();

  uint16_t new_sr = server_.arg("sample_rate").toInt();
  uint8_t new_vol = server_.arg("volume").toInt();

  // Pola zgodne z tym, co parsujesz w read_config()
  doc["ssid"] = new_ssid;
  doc["pass"] = new_pass;
  doc["timezone"] = tz_seconds;

  doc["api_key"] = new_api_key;
  doc["lat"] = new_lat;
  doc["lon"] = new_lon;

  doc["sample_rate"] = new_sr;
  doc["volume"] = new_vol;
}
