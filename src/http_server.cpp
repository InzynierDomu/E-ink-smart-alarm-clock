#include "http_server.h"

#include "config.h"
#include "config_page_style.h"

#include <ArduinoJson.h>
#include <SD.h>


HttpServer::HttpServer(WebServer& server, Clock_model& clock_model, Weather_model& weather_model, Calendar_model& calendar_model,
                       Audio& audio)
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

  server_.on(
      "/upload_firmware",
      HTTP_POST,
      [this]() { server_.send(200, "text/plain", "OK"); },
      [this](HTTPUpload& upload) {
        static File uploadFile;

        if (upload.status == UPLOAD_FILE_START)
        {
          if (SD.exists("/firmware.bin"))
          {
            SD.remove("/firmware.bin");
          }
          uploadFile = SD.open("/firmware.bin", FILE_WRITE);
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          if (uploadFile)
          {
            uploadFile.write(upload.buf, upload.currentSize);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (uploadFile)
          {
            uploadFile.close();
          }
          Serial.printf("Firmware upload complete: %s\n", upload.filename.c_str());
        }
      });

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

  Serial.printf("[HA] Connecting to %s:%u\n", ha_config.ha_host, ha_config.ha_port);
  if (!client.connect(ha_config.ha_host.c_str(), ha_config.ha_port))
  {
    Serial.println("[HA] Connection failed");
    return 0;
  }
  Serial.println("[HA] Connected, sending request");

  String url = "/api/states/" + String(ha_config.ha_enitty_weather_name);
  String request = String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + String(ha_config.ha_host) + "\r\n" + "Authorization: Bearer " +
                   String(ha_config.ha_token) + "\r\n" + "Connection: close\r\n\r\n";

  Serial.println("[HA] Request:");
  Serial.println(request);

  client.print(request);

  String headers;
  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    if (line == "\r")
    {
      break;
    }
    headers += line + "\n";
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
  doc["off_delay"] = 1; // Home Assistant sam przełączy na OFF po 1 sekundzie

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
        <div class="section">
            <div class="section-title">📡 WiFi</div>
            <div class="form-row">
                <label class="form-label">SSID</label>
                <input type="text" name="ssid" value=")rawHTML";
  html += String(wifi.ssid);
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Hasło</label>
                <input type="password" name="pass" value=")rawHTML";
  html += String(wifi.pass);
  html += R"rawHTML(">
            </div>
        </div>
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
        <div class="section">
            <div class="section-title">🕐 Strefa czasowa</div>
            <div class="form-row">
                <label class="form-label">UTC offset [h]</label>
                <input type="number" step="1" name="timezone_hours" value=")rawHTML";
  html += String(timezone_hours);
  html += R"rawHTML(">
            </div>
        </div>
)rawHTML";
  return html;
}

String HttpServer::buildWeatherSection()
{
  Open_weather_config weather;
  weather_model_.get_config(weather);
  String html;
  html += R"rawHTML(
        <div class="section">
            <div class="section-title">🌤️ Pogoda (OpenWeather)</div>
            <div class="form-row">
                <label class="form-label">API key</label>
                <input type="password" name="api_key" value=")rawHTML";
  html += String(weather.api_key);
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Szerokość (lat)</label>
                <input type="text" name="lat" value=")rawHTML";
  html += String(weather.lat);
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Długość (lon)</label>
                <input type="text" name="lon" value=")rawHTML";
  html += String(weather.lon);
  html += R"rawHTML(">
            </div>
        </div>
)rawHTML";
  return html;
}

String HttpServer::buildGoogleCalendarSection()
{
  google_api_config config;
  calendar_model_.get_config(config);

  String html;
  html += R"rawHTML(
        <div class="section">
            <div class="section-title">📅 Google Calendar / Skrypt</div>
            <div class="form-row">
                <label class="form-label">API skryptu Apps</label>
                <input type="password" name="google_script_url" value=")rawHTML";
  html += config.script_url;
  html += R"rawHTML(" placeholder="https://script.google.com/macros/d/...">
            </div>
            <div class="form-row">
                <label class="form-label">ID kalendarza (wszystkie)</label>
                <input type="text" name="google_calendar_id" value=")rawHTML";
  html += config.google_calendar_id;
  html += R"rawHTML(" placeholder="abc123@group.calendar.google.com">
            </div>
            <div class="form-row">
                <label class="form-label">ID kalendarza (alarmy)</label>
                <input type="text" name="google_calendar_alarm_id" value=")rawHTML";
  html += config.alarm_calendar_id;
  html += R"rawHTML(" placeholder="abc@group.calendar.google.com">
            </div>
        </div>
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
        <div class="section">
            <div class="section-title">🔊 Audio</div>
            <div class="form-row">
                <label class="form-label">Sample rate</label>
                <select name="sample_rate">
)rawHTML";
  for (uint8_t i = 0; i < sizeof(sr_list) / sizeof(sr_list[0]); i++)
  {
    html += "<option value=\"" + String(sr_list[i]) + "\"";
    if (sr_list[i] == config.sample_rate)
      html += " selected";
    html += ">" + String(sr_list[i]) + " Hz</option>";
  }
  html += R"rawHTML(
                </select>
            </div>
            <div class="form-row">
                <label class="form-label">Głośność</label>
                <input type="range" name="volume" min="0" max="100" value=")rawHTML";
  html += String(config.volume);
  html += R"rawHTML(" style="width: 100%;">
            </div>
        </div>
)rawHTML";
  return html;
}

String HttpServer::buildHaSection()
{
  String html;
  html += R"rawHTML(
        <div class="section">
            <div class="section-title">🏠 Home Assistant</div>
            <div class="form-row">
                <label class="form-label">Host</label>
                <input type="text" name="ha_host" value=")rawHTML";
  html += ha_config.ha_host;
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Port</label>
                <input type="text" name="ha_port" value=")rawHTML";
  html += String(ha_config.ha_port);
  html += R"rawHTML(" min="1" max="65535">
            </div>
            <div class="form-row">
                <label class="form-label">Token</label>
                <input type="password" name="ha_token" value=")rawHTML";
  html += ha_config.ha_token;
  html += R"rawHTML("> 
            </div>
            <div class="form-row">
                <label class="form-label">User</label>
                <input type="text" name="ha_user" value=")rawHTML";
  html += ha_config.ha_user;
  html += R"rawHTML(">
              </div>
            <div class="form-row">
                <label class="form-label">User pass</label>
                <input type="password" name="ha_pass" value=")rawHTML";
  html += ha_config.ha_pass;
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Encja pogody</label>
                <input type="text" name="ha_entity_weather" value=")rawHTML";
  html += ha_config.ha_enitty_weather_name;
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Encja zegara</label>
                <input type="text" name="ha_entity_clock" value=")rawHTML";
  html += ha_config.ha_entity_clock_name;
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Mqtt port</label>
                <input type="text" name="mqtt_port" value=")rawHTML";
  html += String(ha_config.mqtt_port);
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Pogoda z HA</label>
                <input type="checkbox" name="weather_from_ha" value="1" )rawHTML";
  if (ha_config.weather_from_ha)
    html += "checked";
  html += R"rawHTML(>
            </div>
        </div>
)rawHTML";
  return html;
}

String HttpServer::buildFirmwareUpdateSection()
{
  String html;
  html += R"rawHTML(
  <div class="section">
    <div class="section-title">🛠️ Aktualizacja firmware</div>
    <p>
      Wybierz plik z nowym oprogramowaniem (plik <code>.bin</code>) i wgraj go na urządzenie.
      Po ponownym uruchomieniu, jeśli plik <code>firmware.bin</code> będzie na karcie SD,
      urządzenie spróbuje wykonać aktualizację.
    </p>
    <form method="POST" action="/upload_firmware" enctype="multipart/form-data">
      <div class="form-row">
        <input type="file" name="firmware">
      </div>
      <div class="form-row">
        <button type="submit">Wgraj firmware</button>
      </div>
    </form>
  </div>
  )rawHTML";
  return html;
}

String HttpServer::buildFirmwareUpdateSection()
{
  String html;
  html += R"rawHTML(
  <div class="section">
    <div class="section-title">🛠️ Aktualizacja firmware</div>
    <p>
      Aktualna wersja oprogramowania: <strong>)rawHTML";
  html += FW_VERSION;
  html += R"rawHTML(</strong>
    </p>
    <p>
      Wybierz plik z nowym oprogramowaniem (plik <code>.bin</code>) i wgraj go na urządzenie.
      Po ponownym uruchomieniu, jeśli plik <code>firmware.bin</code> będzie na karcie SD,
      urządzenie spróbuje wykonać aktualizację.
    </p>
    <form method="POST" action="/upload_firmware" enctype="multipart/form-data">
      <div class="form-row">
        <input type="file" name="firmware">
      </div>
      <div class="form-row">
        <button type="submit">Wgraj firmware</button>
      </div>
    </form>
  </div>
  )rawHTML";
  return html;
}

String HttpServer::buildPage()
{
  String page = R"rawHTML(
<!DOCTYPE html>
<html lang="pl">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Konfiguracja - Inteligentny alarm na e-ink</title>
    <link rel="stylesheet" href="/config_page_style.css">
</head>
<body>
    <div class="container">
    <!-- Top bar z logiem -->
        <div class="top-bar">
            <div class="logo-section">
                <img src="https://www.inzynierdomu.pl/wp-content/uploads/2017/01/ID_logo-sygnetblue-1.png" 
                     alt="Inżynier Domu" 
                     class="logo-image"
                     width="50" 
                     height="50">
                <div class="logo-text">
                    <h1>Konfiguracja urządzenia</h1>
                </div>
            </div>
            <div class="header-spacer"></div>
        </div>

        <!-- Formularz -->
        <form method="POST" action="/save">
)rawHTML";

  page += buildWifiSection();
  // page += buildTimezoneSection(); TODO fix
  page += buildWeatherSection();
  page += buildGoogleCalendarSection();
  page += buildHaSection();
  page += buildAudioSection();
  page += buildFirmwareUpdateSection();

  page += R"rawHTML(
            <div class="button-group">
                <button type="submit">Zapisz konfigurację</button>
            </div>
        </form>
)rawHTML";

  page += buildFooter();

  page += R"rawHTML(
    </div>
</body>
</html>
)rawHTML";

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

  String new_google_script_url = server_.arg("google_script_url");
  String new_google_calendar_id = server_.arg("google_calendar_id");
  String new_google_calendar_alarm_id = server_.arg("google_calendar_alarm_id");

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

  doc["google_script_url"] = new_google_script_url;
  doc["google_calendar_id"] = new_google_calendar_id;
  doc["google_calendar_alarm_id"] = new_google_calendar_alarm_id;

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