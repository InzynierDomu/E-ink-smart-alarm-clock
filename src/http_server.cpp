/**
 * @file http_server.cpp
 * @brief Implementation of the HTTP server — configuration page, settings persistence, logging, and HA/MQTT integration.
 */

#include "http_server.h"

#include "config.h"
#include "config_page_style.h"
#include "logger.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <freertos/semphr.h>


extern SemaphoreHandle_t g_sd_mutex;


/**
 * @brief Initializes the HTTP server with references to models and the audio object.
 * @param server Reference to the WebServer object.
 * @param clock_model Reference to the clock model.
 * @param weather_model Reference to the weather model.
 * @param calendar_model Reference to the calendar model.
 * @param audio Reference to the Audio object.
 */
HttpServer::HttpServer(WebServer& server, Clock_model& clock_model, Weather_model& weather_model, Calendar_model& calendar_model,
                       Audio& audio)
: server_(server)
, clock_model_(clock_model)
, weather_model_(weather_model)
, calendar_model_(calendar_model)
, audio_(audio)
, mqtt_client(client)
{}

/**
 * @brief Registers HTTP handlers and starts the web server.
 */
void HttpServer::begin()
{
  server_.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server_.on("/save", HTTP_POST, [this]() { this->handleSave(); });
  server_.on("/config_page_style.css", HTTP_GET, [this]() { server_.send(200, "text/css", config_page_style_css); });
  server_.on("/logs", HTTP_GET, [this]() { this->handleLogs(); });
  server_.on("/logs/clear", HTTP_POST, [this]() { this->handleLogsClear(); });

  server_.on(
      "/upload_firmware",
      HTTP_POST,
      [this]() {
        server_.send(200,
                     "text/plain; charset=utf-8",
                     "Firmware zapisany na SD jako /firmware.bin. Zrestartuj urzadzenie. Po ponownym uruchomieniu, urządzenie spróbuje "
                     "wykonać aktualizację.");
      },
      [this]() {
        HTTPUpload& upload = server_.upload();
        static File uploadFile;

        if (upload.status == UPLOAD_FILE_START)
        {
          if (SD.exists("/firmware.bin"))
          {
            SD.remove("/firmware.bin");
          }
          uploadFile = SD.open("/firmware.bin", FILE_WRITE);
          if (!uploadFile)
          {
            Serial.println("Nie mozna otworzyc /firmware.bin do zapisu");
          }
          else
          {
            Serial.printf("Start uploadu firmware: %s\n", upload.filename.c_str());
          }
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
          Serial.printf("Firmware upload complete: %s, size=%u\n", upload.filename.c_str(), upload.totalSize);
        }
        else if (upload.status == UPLOAD_FILE_ABORTED)
        {
          if (uploadFile)
          {
            uploadFile.close();
          }
          if (SD.exists("/firmware.bin"))
          {
            SD.remove("/firmware.bin");
          }
          Serial.println("Upload firmware przerwany");
        }
      });

  server_.begin();
}

/**
 * @brief Registers a catch-all redirect handler for captive portal detection on iOS, Android and Windows.
 */
void HttpServer::setup_captive_portal()
{
  server_.on("/redirect", HTTP_GET, [this]() {
    server_.sendHeader("Location", "http://192.168.4.1/");
    server_.send(302, "text/html", "<a href='http://192.168.4.1/'>Konfiguracja</a>");
  });

  server_.onNotFound([this]() {
    server_.sendHeader("Location", "http://192.168.4.1/");
    server_.send(302, "text/html", "<a href='http://192.168.4.1/'>Konfiguracja</a>");
  });
}

/**
 * @brief Stores the Home Assistant configuration in the HTTP server.
 * @param config Reference to the HA_config structure with configuration data.
 */
void HttpServer::ha_set_config(HA_config& config)
{
  ha_config = config;
}

/**
 * @brief Retrieves the current temperature from a Home Assistant weather entity via HTTP.
 * @return Temperature in degrees Celsius, or 0 on error.
 */
/**
 * @brief Builds an HTTP GET request string for the HA weather entity.
 * @return Full HTTP/1.1 GET request string including authorization header.
 */
String HttpServer::build_ha_request() const
{
  String url = "/api/states/" + String(ha_config.ha_enitty_weather_name);
  return String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + String(ha_config.ha_host) + "\r\n" + "Authorization: Bearer " +
         String(ha_config.ha_token) + "\r\n" + "Connection: close\r\n\r\n";
}

/**
 * @brief Retrieves the current temperature from a Home Assistant weather entity via HTTP.
 * @return Temperature in degrees Celsius, or 0 on error.
 */
int8_t HttpServer::get_ha_weather()
{
  Serial.println("=== updateHaMeasurement ===");

  if (ha_config.ha_host.isEmpty())
  {
    return 0;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Logger::warn("HA", "WiFi not connected, skipping weather fetch");
    return 0;
  }

  Serial.printf("[HA] Connecting to %s:%u\n", ha_config.ha_host, ha_config.ha_port);
  if (!client.connect(ha_config.ha_host.c_str(), ha_config.ha_port, 10000))
  {
    Logger::error("HA", "Connection failed to " + ha_config.ha_host + ":" + String(ha_config.ha_port));
    return 0;
  }

  client.setTimeout(10);
  client.print(build_ha_request());

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

  if (!is_ha_response_ok(headers))
  {
    Logger::error("HA", "Non-200 response: " + headers.substring(0, headers.indexOf('\n')));
    client.stop();
    return 0;
  }

  String body;
  while (client.available())
  {
    body += client.readString();
  }
  client.stop();

  return parse_ha_state(body);
}

/**
 * @brief Checks whether weather data should be fetched from Home Assistant.
 * @return true if weather from HA is configured.
 */
bool HttpServer::is_weather_from_ha()
{
  return ha_config.weather_from_ha;
}

/**
 * @brief Registers the clock entity in Home Assistant via MQTT Discovery.
 */
void HttpServer::entity_clock_setup()
{
  mqtt_client.setServer(ha_config.ha_host.c_str(), ha_config.mqtt_port);
  mqtt_client.setBufferSize(512);

  if (!mqtt_client.connected())
  {
    mqtt_reconnect();
    delay(300);
  }

  DynamicJsonDocument doc(512);
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

  if (!mqtt_client.publish(topic.c_str(), msg.c_str(), true))
  {
    Logger::error("HA-MQTT", "Publish failed (state: " + String(mqtt_client.state()) + ")");
  }
}

/**
 * @brief Builds the HTML section for the WiFi configuration form.
 * @return String containing the HTML for the WiFi section.
 */
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


/**
 * @brief Sends an MQTT "ON" message to the clock entity state topic in Home Assistant.
 */
void HttpServer::send_mqtt_action()
{
  if (!mqtt_client.connected())
  {
    mqtt_reconnect();
    delay(300);
  }

  String topic = "eink_clock/" + String(ha_config.ha_entity_clock_name) + "/state";

  if (!mqtt_client.publish(topic.c_str(), "ON", false))
  {
    Logger::error("HA-MQTT", "Publish failed (state: " + String(mqtt_client.state()) + ")");
  }
}

// String HttpServer::buildTimezoneSection()
// {
//   Wifi_Config wifi;
//   clock_model_.get_wifi_config(wifi);
//   int timezone_hours = wifi.timezone / 3600;


//   String html;
//   html += R"rawHTML(
// <fieldset>
// <legend>🕐 Strefa czasowa</legend>
// <label>UTC offset [h]</label> <input type="number" step="1" name="timezone_hours" value=")rawHTML";
//   html += String(timezone_hours);
//   html += R"rawHTML(">
// </fieldset>
// </fieldset>
// )rawHTML";
//   return html;
// }

/**
 * @brief Builds the HTML section for the OpenWeather configuration form.
 * @return String containing the HTML for the weather section.
 */
String HttpServer::buildWeatherSection()
{
  Open_weather_config weather;
  weather_model_.get_config(weather);
  String html;
  html += R"rawHTML(
        <div class="section">
            <div class="section-title">🌤️ Pogoda (OpenWeather)</div>
            <div class="form-row">
                <label class="form-label">Klucz API uzyskasz po rejestracji na <a href="https://openweathermap.org/" target="_blank" rel="noopener">openweathermap.org</a>.</label>
            </div>
            <div class="form-row">
                <label class="form-label">API key</label>
                <input type="password" name="api_key" value=")rawHTML";
  html += String(weather.api_key);
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Szerokość (lat)</label>
                <input type="text" name="lat" value=")rawHTML";
  html += String(weather.lat, 5);
  html += R"rawHTML(">
            </div>
            <div class="form-row">
                <label class="form-label">Długość (lon)</label>
                <input type="text" name="lon" value=")rawHTML";
  html += String(weather.lon, 5);
  html += R"rawHTML(">
            </div>
        </div>
)rawHTML";
  return html;
}

/**
 * @brief Builds the HTML section for the Google Calendar (iCal) configuration form.
 * @return String containing the HTML for the calendar section.
 */
String HttpServer::buildGoogleCalendarSection()
{
  google_api_config cal_config;
  calendar_model_.get_config(cal_config);
  String html;
  html += R"rawHTML(
    <div class="section">
        <div class="section-title">📅 Google Calendar (iCal)</div>
        <div class="form-row">
            <label class="form-label">W ustawieniach Google Calendar skopiuj <b>Tajny adres w formacie iCal</b> dla każdego kalendarza.</label>
        </div>
        <div class="form-row">
            <label class="form-label">URL kalendarza (wydarzenia)</label>
            <input type="password" name="ical_url" value=")rawHTML";
  html += cal_config.ical_url;
  html += R"rawHTML(">
        </div>
        <div class="form-row">
            <label class="form-label">URL kalendarza alarmów</label>
            <input type="password" name="ical_alarm_url" value=")rawHTML";
  html += cal_config.ical_alarm_url;
  html += R"rawHTML(">
        </div>
        <div class="form-row">
            <label class="form-label">Automatyczne wyłączenie alarmu (5 min.)</label>
            <input type="checkbox" name="alarm_auto_stop")rawHTML";
  if (alarm_auto_stop_)
    html += " checked";
  html += R"rawHTML(>
        </div>
    </div>
)rawHTML";
  return html;
}
/**
 * @brief Builds the HTML section for the audio configuration form (sample rate and volume).
 * @return String containing the HTML for the audio section.
 */
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

/**
 * @brief Builds the HTML section for the Home Assistant and MQTT configuration form.
 * @return String containing the HTML for the Home Assistant section.
 */
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

/**
 * @brief Builds the HTML section for uploading firmware via the browser.
 * @return String containing the HTML for the firmware update section.
 */
String HttpServer::buildFirmwareUpdateSection()
{
  String html;
  html += R"rawHTML(
  <br><div class="section">
    <div class="section-title">🛠️ Aktualizacja firmware</div>
    <div class="form-row">
      <label class="form-label">Obecna wersja: </label><label class="form-label">)rawHTML";
  html += config::version;
  html +=
      R"rawHTML(</label></div><div class="form-row"><label class="form-label">Wybierz plik z nowym oprogramowaniem (*<code>.bin</code>).</label>
    <form id="firmware-form" method="POST" action="/upload_firmware" enctype="multipart/form-data">
        <input type="file" name="firmware" id="firmware-file">
    </div>
      <div class="form-row">
        <button type="submit" id="firmware-submit" disabled>Wgraj firmware</button>
      </div>
    </form>
  </div>
  )rawHTML";
  return html;
}

/**
 * @brief Builds the HTML section with buttons for downloading and clearing logs.
 * @return String containing the HTML for the logs section.
 */
String HttpServer::buildLogsSection()
{
  String html;
  html += R"rawHTML(
  <div class="section">
    <div class="section-title">📋 Logi</div>
    <div class="form-row" style="gap:8px; flex-wrap:wrap;">
      <a href="/logs" download="logs.txt"><button type="button">Pobierz logi</button></a>
      <form method="POST" action="/logs/clear" style="margin:0;">
        <button type="submit" style="width:auto; padding-left:24px; padding-right:24px;" onclick="return confirm('Wyczyścić plik logów?')">Wyczyść logi</button>
      </form>
    </div>
  </div>
  )rawHTML";
  return html;
}

/**
 * @brief Handles the GET /logs request — sends the log file as a downloadable attachment.
 */
void HttpServer::handleLogs()
{
  if (!SD.exists(Logger::path()))
  {
    server_.send(404, "text/plain; charset=utf-8", "Brak pliku logów.");
    return;
  }

  String content;
  if (xSemaphoreTake(g_sd_mutex, pdMS_TO_TICKS(5000)) == pdTRUE)
  {
    File f = SD.open(Logger::path(), "r");
    if (f)
    {
      content = f.readString();
      f.close();
    }
    xSemaphoreGive(g_sd_mutex);
  }

  if (content.isEmpty())
  {
    server_.send(500, "text/plain; charset=utf-8", "Nie można odczytać pliku logów.");
    return;
  }

  server_.sendHeader("Content-Disposition", "attachment; filename=\"logs.txt\"");
  server_.send(200, "text/plain; charset=utf-8", content);
}

/**
 * @brief Handles the POST /logs/clear request — deletes the log file and redirects to the main page.
 */
void HttpServer::handleLogsClear()
{
  if (SD.exists(Logger::path()))
  {
    SD.remove(Logger::path());
  }
  server_.sendHeader("Location", "/");
  server_.send(303);
}

/**
 * @brief Builds the HTML footer of the configuration page with links and icons.
 * @return String containing the HTML for the footer.
 */
String HttpServer::buildFooter()
{
  String html;
  html += R"rawHTML(
        <div class="footer">
            <p>&copy; 2026 <a href="https://inzynierdomu.pl/kontakt/" target="_blank" rel="noopener" style="color:#60a5fa;">Inżynier Domu</a>. Wszystkie prawa zastrzeżone.</p>
            <p style="margin-top:8px;font-size:0.9em;"><a href="https://inzynierdomu.pl/budzik-instrukcja" target="_blank" rel="noopener" style="color:#60a5fa;">Instrukcja konfiguracji</a></p>
        </div>
)rawHTML";
  return html;
}

/**
 * @brief Handles the GET / request — generates and sends the configuration page using chunked transfer.
 */
void HttpServer::handleRoot()
{
  server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server_.send(200, "text/html; charset=utf-8", "");

  // chunk 1: head + CSS
  server_.sendContent("<!DOCTYPE html><html lang=\"pl\"><head>"
                      "<meta charset=\"UTF-8\">"
                      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
                      "<title>Konfiguracja - Inteligentny alarm na e-ink</title>"
                      "<style>");
  server_.sendContent(config_page_style_css);

  // chunk 2: body open + loading overlay + form header
  server_.sendContent("</style></head><body>"
                      "<div id=\"loading-overlay\" class=\"loading-overlay\">"
                      "<div class=\"spinner\"></div>"
                      "<div class=\"overlay-text\">Ładowanie&hellip;</div>"
                      "</div>"
                      "<div class=\"container\">"
                      "<div class=\"top-bar\"><div class=\"logo-section\">"
                      "<div class=\"logo-text\"><h1>Konfiguracja urządzenia</h1></div>"
                      "</div><div class=\"header-spacer\"></div></div>"
                      "<form method=\"POST\" action=\"/save\">");

  // chunk 3: all config sections + save button in one go
  server_.sendContent(buildWifiSection() + buildWeatherSection() + buildGoogleCalendarSection() + buildHaSection() +
                      "<div class=\"button-group\">"
                      "<button type=\"submit\">Zapisz konfigurację</button>"
                      "</div></form>");

  // chunk 4: firmware + logs + footer + scripts
  server_.sendContent(buildFirmwareUpdateSection() + buildLogsSection() + buildFooter() +
                      "<div id=\"upload-overlay\" class=\"upload-overlay\">"
                      "<div class=\"spinner\"></div>"
                      "<div class=\"overlay-text\">Wgrywanie firmware&hellip;<br>Proszę czekać, nie zamykaj strony.</div>"
                      "</div>"
                      "<script>"
                      "document.getElementById('loading-overlay').classList.add('hidden');"
                      "document.getElementById('firmware-file').addEventListener('change',function(){"
                      "document.getElementById('firmware-submit').disabled=!this.files.length;"
                      "});"
                      "document.getElementById('firmware-form').addEventListener('submit',function(){"
                      "document.getElementById('upload-overlay').classList.add('active');"
                      "});"
                      "</script>"
                      "</body></html>");

  server_.sendContent("");
}

/**
 * @brief Handles the POST /save request — saves the form configuration to the SD card and restarts the ESP.
 */
void HttpServer::handleSave()
{
  Serial.printf("[SAVE] ssid from form: '%s'\n", server_.arg("ssid").c_str());
  JsonDocument doc;
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
  Logger::info("BOOT", "Config saved via web, restarting");
  server_.send(200, "text/plain", "Zapisano, restartuje...");
  delay(500);
  ESP.restart();
}

/**
 * @brief Loads the JSON configuration file from the SD card into an ArduinoJson document.
 * @param doc Reference to the JSON document that will receive the configuration.
 * @return true if loading succeeded or the file does not exist, false on parse error.
 */
bool HttpServer::loadConfigJson(JsonDocument& doc)
{
  File file = SD.open(config::config_path, "r");
  if (!file)
    return true; // brak pliku — zacznij od pustego doca
  String jsonData;
  while (file.available())
    jsonData += (char)file.read();
  file.close();
  if (jsonData.length() == 0)
    return true; // pusty plik — zacznij od pustego doca
  DeserializationError err = deserializeJson(doc, jsonData);
  return !err;
}

/**
 * @brief Saves the JSON configuration document to the SD card.
 * @param doc Reference to the JSON document containing the configuration to save.
 * @return true if saving succeeded, false on error.
 */
bool HttpServer::saveConfigJson(const JsonDocument& doc)
{
  if (SD.exists(config::config_path))
    SD.remove(config::config_path);
  File file = SD.open(config::config_path, FILE_WRITE);
  if (!file)
  {
    Serial.println("saveConfigJson: nie mozna otworzyc pliku do zapisu");
    return false;
  }
  size_t written = serializeJson(doc, file);
  file.close();
  Serial.printf("saveConfigJson: zapisano %u bajtow\n", written);
  return written > 0;
}

/**
 * @brief Populates the JSON document with values from the HTTP POST request parameters.
 * @param doc Reference to the JSON document to be updated with form data.
 */
void HttpServer::updateConfigFromRequest(JsonDocument& doc)
{
  String new_ssid = server_.arg("ssid");
  String new_pass = server_.arg("pass");
  // int tz_hours = server_.arg("timezone_hours").toInt(); TODO fix
  // int tz_seconds = tz_hours * 3600;
  String new_api_key = server_.arg("api_key");
  float new_lat = server_.arg("lat").toFloat();
  float new_lon = server_.arg("lon").toFloat();
  // uint16_t new_sr = server_.arg("sample_rate").toInt();  // audio hard-coded
  // uint8_t new_vol = server_.arg("volume").toInt();       // audio hard-coded
  String new_ha_host = server_.arg("ha_host");
  uint16_t new_ha_port = server_.arg("ha_port").toInt();
  String new_ha_token = server_.arg("ha_token");
  String new_ha_user = server_.arg("ha_user");
  String new_ha_pass = server_.arg("ha_pass");
  String new_ha_entity_weather = server_.arg("ha_entity_weather");
  String new_ha_entity_clock = server_.arg("ha_entity_clock");
  uint16_t new_mqtt_port = server_.arg("mqtt_port").toInt();
  bool new_weather_from_ha = server_.arg("weather_from_ha").toInt();

  String new_ical_url = server_.arg("ical_url");
  String new_ical_alarm_url = server_.arg("ical_alarm_url");

  doc["ssid"] = new_ssid;
  doc["pass"] = new_pass;
  doc["ical_url"] = new_ical_url;
  doc["ical_alarm_url"] = new_ical_alarm_url;
  // doc["timezone"] = tz_seconds; TODO fix
  doc["openweathermap_api_key"] = new_api_key;
  doc["lat"] = new_lat;
  doc["lon"] = new_lon;
  // doc["sample_rate"] = new_sr;  // audio hard-coded
  // doc["volume"] = new_vol;       // audio hard-coded
  doc["HA_host"] = new_ha_host;
  doc["HA_port"] = new_ha_port;
  doc["HA_token"] = new_ha_token;
  doc["HA_user"] = new_ha_user;
  doc["HA_pass"] = new_ha_pass;
  doc["HA_weather_entity_name"] = new_ha_entity_weather;
  doc["HA_clock_entity_name"] = new_ha_entity_clock;
  doc["mqtt_port"] = new_mqtt_port;
  doc["weather_from_HA"] = new_weather_from_ha;
  bool new_alarm_auto_stop = server_.hasArg("alarm_auto_stop");
  doc["alarm_auto_stop"] = new_alarm_auto_stop;
  alarm_auto_stop_ = new_alarm_auto_stop;
}

static String mqtt_state_description(int state)
{
  switch (state)
  {
    case -4:
      return "connection timeout";
    case -3:
      return "connection lost";
    case -2:
      return "connect failed";
    case -1:
      return "disconnected";
    case 1:
      return "bad protocol";
    case 2:
      return "bad client ID";
    case 3:
      return "server unavailable";
    case 4:
      return "bad credentials";
    case 5:
      return "unauthorized";
    default:
      return "unknown";
  }
}

/**
 * @brief Attempts to reconnect the MQTT client to the broker (up to 3 retries).
 */
void HttpServer::mqtt_reconnect()
{
  uint8_t attempts = 0;
  while (!mqtt_client.connected() && attempts < 3)
  {
    String clientId = "Eink_Clock_" + String(random(0xffff), HEX);
    if (!mqtt_client.connect(clientId.c_str(), ha_config.ha_user.c_str(), ha_config.ha_pass.c_str()))
    {
      int state = mqtt_client.state();
      Logger::error("HA-MQTT", "Connect failed: " + String(state) + " (" + mqtt_state_description(state) + ")");
      delay(5000);
      attempts++;
    }
  }
  if (!mqtt_client.connected())
    Logger::error("HA-MQTT", "Max attempts reached, skipping");
}
