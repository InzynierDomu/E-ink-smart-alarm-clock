#include "http_server.h"
#include "config.h"
#include "config_page_style.h"
#include <ArduinoJson.h>
#include <SD.h>

HttpServer::HttpServer(WebServer& server, Clock_model& clock_model, Weather_model& weather_model, Calendar_model& calendar_model, Audio& audio)
: server_(server)
, clock_model_(clock_model)
, weather_model_(weather_model)
, calendar_model_(calendar_model)
, audio_(audio)
, mqtt_client(client)
{}

void HttpServer::begin()
{
  server_.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server_.on("/save", HTTP_POST, [this]() { this->handleSave(); });
  server_.on("/config_page_style.css", HTTP_GET, [this]() { server_.send(200, "text/css", config_page_style_css); });
  server_.begin();
}

void HttpServer::ha_set_config(HA_config& config)
{
  ha_config = config;
}

int8_t HttpServer::get_ha_weather()
{
  String haTemperature = "--";
  Serial.println("=== updateHaMeasurement ===");

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[HA] WiFi not connected");
    return 0;
  }

  Serial.printf("[HA] Connecting to %s:%u
", ha_config.ha_host, ha_config.ha_port);
  if (!client.connect(ha_config.ha_host.c_str(), ha_config.ha_port))
  {
    Serial.println("[HA] Connection failed");
    return 0;
  }

  Serial.println("[HA] Connected, sending request");
  String url = "/api/states/" + String(ha_config.ha_enitty_weather_name);
  String request = String("GET ") + url + " HTTP/1.1\r
" + "Host: " + String(ha_config.ha_host) + "\r
" + "Authorization: Bearer " + String(ha_config.ha_token) + "\r
" + "Connection: close\r
\r
";

  Serial.println("[HA] Request:");
  Serial.println(request);

  client.print(request);

  String headers;
  while (client.connected())
  {
    String line = client.readStringUntil('
');
    if (line == "\r")
    {
      break;
    }
    headers += line + "
";
  }

  Serial.println("[HA] Headers:");
  Serial.println(headers);

  if (!headers.startsWith("HTTP/1.1 200"))
  {
    Serial.println("[HA] Non-200 response");
    return 0;
  }

  String body;
  while (client.available())
  {
    body += client.readString();
  }
  client.stop();

  Serial.println("[HA] Body:");
  Serial.println(body);

  int idx = body.indexOf("\"state\":");
  if (idx < 0)
  {
    Serial.println("[HA] 'state' not found in JSON");
    return 0;
  }

  int start = body.indexOf("\"", idx + 8);
  int end = body.indexOf("\"", start + 1);

  if (start < 0 || end < 0 || end <= start)
  {
    Serial.println("[HA] Failed to parse state string");
    return 0;
  }

  haTemperature = body.substring(start + 1, end);
  Serial.print("[HA] Parsed state: ");
  Serial.println(haTemperature);

  return haTemperature.toInt();
}

bool HttpServer::is_weather_from_ha()
{
  return ha_config.weather_from_ha;
}

void HttpServer::entity_clock_setup()
{
  mqtt_client.setServer(ha_config.ha_host.c_str(), ha_config.mqtt_port);
  mqtt_client.setBufferSize(1024);

  if (!mqtt_client.connected())
  {
    Serial.println("MQTT niepołączone, próba rekonakcji...");
    mqtt_reconnect();
    delay(1000);
  }

  DynamicJsonDocument doc(1024);
  doc["name"] = ha_config.ha_entity_clock_name;
  doc["unique_id"] = ha_config.ha_entity_clock_name; // Musi być unikalne w skali całego HA
  doc["state_topic"] = "eink_clock/" + ha_config.ha_entity_clock_name + "/state";
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["device_class"] = "motion"; // "motion" pasuje do jednorazowych zdarzeń (jak czujnik ruchu)
  doc["off_delay"] = 1;           // Home Assistant sam przełączy na OFF po 1 sekundzie

  JsonObject device = doc.createNestedObject("device");
  device["identifiers"][0] = "eink_clock_" + ha_config.ha_entity_clock_name;
  device["name"] = ha_config.ha_entity_clock_name;
  device["model"] = "ESP32 e-ink";
  device["manufacturer"] = "inzynier domu";

  String msg;
  serializeJson(doc, msg);

  String topic = "homeassistant/binary_sensor/" + ha_config.ha_entity_clock_name + "/config";
  Serial.print("Wysyłanie konfiguracji do: ");
  Serial.println(topic);

  if (mqtt_client.publish(topic.c_str(), msg.c_str(), true))
  {
    Serial.println("Konfiguracja wysłana pomyślnie!");
  }
  else
  {
    Serial.println("Błąd wysyłania! Sprawdź wielkość bufora (setBufferSize).");
  }
}

String HttpServer::buildWifiSection()
{
  Wifi_Config wifi;
  clock_model_.get_wifi_config(wifi);

  String html;
  html += R"rawHTML(
<fieldset>
<legend>📡 WiFi</legend>
<label>SSID</label> <input type="text" name="ssid" value=")rawHTML" ;
  html += String(wifi.ssid);
  html += R"rawHTML(">
<label>Hasło</label> <input type="password" name="pass" value=")rawHTML" ;
  html += String(wifi.pass);
  html += R"rawHTML(">
</fieldset>
)rawHTML";
  return html;
}

void HttpServer::send_mqtt_action()
{
  // 1. Sprawdź, czy w ogóle jesteśmy połączeni z MQTT
  if (!mqtt_client.connected())
  {
    Serial.println("MQTT niepołączone, próba rekonakcji...");
    mqtt_reconnect();
    delay(1000);
  }

  // 2. Zbuduj topik stanu (taki sam jak w konfiguracji Discovery)
  String topic = "eink_clock/" + String(ha_config.ha_entity_clock_name) + "/state";

  // 3. Wyślij wiadomość "ON"
  // Parametr 'false' na końcu oznacza, że wiadomość NIE jest typu 'retained'.
  // Przy akcjach typu przycisk/ruch lepiej nie używać retain,
  // żeby HA nie odczytał tego starego stanu po swoim restarcie.
  if (mqtt_client.publish(topic.c_str(), "ON", false))
  {
    Serial.println("Akcja MQTT 'ON' wysłana do: " + topic);
  }
  else
  {
    Serial.println("Błąd wysyłania publish! Sprawdź połączenie.");
  }
}

String HttpServer::buildTimezoneSection()
{
  Wifi_Config wifi;
  clock_model_.get_wifi_config(wifi);
  int timezone_hours = wifi.timezone / 3600;

  String html;
  html += R"rawHTML(
<fieldset>
<legend>🕐 Strefa czasowa</legend>
<label>UTC offset [h]</label> <input type="number" step="1" name="timezone_hours" value=")rawHTML" ;
  html += String(timezone_hours);
  html += R"rawHTML(">
</fieldset>
)rawHTML";
  return html;
}

String HttpServer::buildWeatherSection()
{
  Open_weather_config weather;
  weather_model_.get_config(weather);

  String html;
  html += R"rawHTML(
<fieldset>
<legend>🌤️ Pogoda (OpenWeather)</legend>
<label>API key</label> <input type="password" name="api_key" value=")rawHTML" ;
  html += String(weather.api_key);
  html += R"rawHTML(">
<label>Szerokość (lat)</label> <input type="text" name="lat" value=")rawHTML" ;
  html += String(weather.lat);
  html += R"rawHTML(">
<label>Długość (lon)</label> <input type="text" name="lon" value=")rawHTML" ;
  html += String(weather.lon);
  html += R"rawHTML(">
</fieldset>
)rawHTML";
  return html;
}

String HttpServer::buildGoogleCalendarSection()
{
  String html;
  html += R"rawHTML(
<fieldset>
<legend>📅 Google Calendar</legend>
<p>Połącz swój zegar z kalendarzem Google, aby wyświetlać nadchodzące wydarzenia.</p>
<a href="https://inzynierdomu.pl/clock-api/pair.php?device_id=)rawHTML" ;
  html += device_id_;
  html += R"rawHTML(" target="_blank" class="button">Połącz z Google Calendar</a>
</fieldset>
)rawHTML";
  return html;
}

String HttpServer::buildAudioSection()
{
  Audio_config config;
  audio_.get_config(config);
  const uint16_t sr_list[] = {8000, 16000, 22050, 32000, 44100, 48000};

  String html;
  html += R"rawHTML(
<fieldset>
<legend>🔊 Audio</legend>
<label>Sample rate</label> <select name="sample_rate">)rawHTML";
  for (uint8_t i = 0; i < sizeof(sr_list) / sizeof(sr_list[0]); i++)
  {
    html += "<option value=\"" + String(sr_list[i]) + "\"";
    if (sr_list[i] == config.sample_rate)
      html += " selected";
    html += ">" + String(sr_list[i]) + " Hz</option>";
  }
  html += R"rawHTML(</select>
<label>Głośność</label> <input type="range" name="volume" min="0" max="100" value=")rawHTML" ;
  html += String(config.volume);
  html += R"rawHTML(" style="width: 100%;">
</fieldset>
)rawHTML";
  return html;
}

String HttpServer::buildHaSection()
{
  String html;
  html += R"rawHTML(
<fieldset>
<legend>🏠 Home Assistant</legend>
<label>Host</label> <input type="text" name="ha_host" value=")rawHTML" ;
  html += ha_config.ha_host;
  html += R"rawHTML(">
<label>Port</label> <input type="text" name="ha_port" value=")rawHTML" ;
  html += String(ha_config.ha_port);
  html += R"rawHTML(" min="1" max="65535">
<label>Token</label> <input type="password" name="ha_token" value=")rawHTML" ;
  html += ha_config.ha_token;
  html += R"rawHTML(">
<label>User</label> <input type="text" name="ha_user" value=")rawHTML" ;
  html += ha_config.ha_user;
  html += R"rawHTML(">
<label>User pass</label> <input type="password" name="ha_pass" value=")rawHTML" ;
  html += ha_config.ha_pass;
  html += R"rawHTML(">
<label>Encja pogody</label> <input type="text" name="ha_entity_weather" value=")rawHTML" ;
  html += ha_config.ha_enitty_weather_name;
  html += R"rawHTML(">
<label>Encja zegara</label> <input type="text" name="ha_entity_clock" value=")rawHTML" ;
  html += ha_config.ha_entity_clock_name;
  html += R"rawHTML(">
<label>Mqtt port</label> <input type="text" name="mqtt_port" value=")rawHTML" ;
  html += String(ha_config.mqtt_port);
  html += R"rawHTML(">
<label>Pogoda z HA</label> <input type="checkbox" name="weather_from_ha" value="1" )rawHTML";
  if (ha_config.weather_from_ha)
    html += "checked";
  html += R"rawHTML(">
</fieldset>
)rawHTML";
  return html;
}

String HttpServer::buildFooter()
{
  String html;
  html += R"rawHTML(
<footer>
<p>&copy; 2026 Inżynier Domu. Wszystkie prawa zastrzeżone.</p>
<div class="links">
<a href="https://github.com/InzynierDomu/E-ink-smart-alarm-clock">GitHub</a>
<a href="https://buycoffee.to/inzynier-domu">Postaw kawę</a>
<a href="https://www.inzynierdomu.pl/">Blog</a>
<a href="https://www.youtube.com/c/InzynierDomu?sub_confirmation=1">YouTube</a>
</div>
</footer>
)rawHTML";
  return html;
}

String HttpServer::buildPage()
{
  String page = R"rawHTML(<!DOCTYPE html><html><head>
<meta charset="UTF-8"><title>Konfiguracja - Inteligentny alarm na e-ink</title>
<link rel="stylesheet" href="/config_page_style.css">
</head><body>
<header><h1>Inżynier Domu</h1></header>
<main>
<h2>Konfiguracja urządzenia</h2>
<form action="/save" method="POST">
)rawHTML";

  page += buildWifiSection();
  // page += buildTimezoneSection(); TODO fix
  page += buildWeatherSection();
  page += buildGoogleCalendarSection();
  page += buildHaSection();
  page += buildAudioSection();

  page += R"rawHTML(
<button type="submit">Zapisz konfigurację</button>
</form>
</main>
)rawHTML";
  page += buildFooter();
  page += R"rawHTML(</body></html>)rawHTML";

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
  // int tz_hours = server_.arg("timezone_hours").toInt(); TODO fix
  // int tz_seconds = tz_hours * 3600;
  String new_api_key = server_.arg("api_key");
  float new_lat = server_.arg("lat").toFloat();
  float new_lon = server_.arg("lon").toFloat();
  uint16_t new_sr = server_.arg("sample_rate").toInt();
  uint8_t new_vol = server_.arg("volume").toInt();
  String new_ha_host = server_.arg("ha_host");
  uint16_t new_ha_port = server_.arg("ha_port").toInt();
  String new_ha_token = server_.arg("ha_token");
  String new_ha_user = server_.arg("ha_user");
  String new_ha_pass = server_.arg("ha_pass");
  String new_ha_entity_weather = server_.arg("ha_entity_weather");
  String new_ha_entity_clock = server_.arg("ha_entity_clock");
  uint16_t new_mqtt_port = server_.arg("mqtt_port").toInt();
  bool new_weather_from_ha = server_.arg("weather_from_ha").toInt();

  doc["ssid"] = new_ssid;
  doc["pass"] = new_pass;
  // doc["timezone"] = tz_seconds; TODO fix
  doc["openweathermap_api_key"] = new_api_key;
  doc["lat"] = new_lat;
  doc["lon"] = new_lon;
  doc["sample_rate"] = new_sr;
  doc["volume"] = new_vol;
  doc["HA_host"] = new_ha_host;
  doc["HA_port"] = new_ha_port;
  doc["HA_token"] = new_ha_token;
  doc["HA_user"] = new_ha_user;
  doc["HA_pass"] = new_ha_pass;
  doc["HA_weather_entity_name"] = new_ha_entity_weather;
  doc["HA_clock_entity_name"] = new_ha_entity_clock;
  doc["mqtt_port"] = new_mqtt_port;
  doc["weather_from_HA"] = new_weather_from_ha;
}

void HttpServer::mqtt_reconnect()
{
  while (!mqtt_client.connected())
  {
    Serial.print("Próba połączenia MQTT...");
    // LOGIN I HASŁO WPISUJESZ TUTAJ:
    String clientId = "Eink_Clock_" + String(random(0xffff), HEX);
    if (mqtt_client.connect(clientId.c_str(), ha_config.ha_user.c_str(), ha_config.ha_pass.c_str()))
    {
      Serial.println("Połączono!");
    }
    else
    {
      Serial.print("Błąd: ");
      Serial.print(mqtt_client.state());
      Serial.println(" ponowna próba za 5 sekund");
      delay(5000);
    }
  }
}
