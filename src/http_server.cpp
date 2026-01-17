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

int8_t HttpServer::get_ha_weather()
{
  String haTemperature = "--";
  Serial.println("=== updateHaMeasurement ===");

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[HA] WiFi not connected");
    return 0;
  }

  WiFiClient client;
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
                <label class="form-label">Encja pogody</label>
                <input type="text" name="ha_entity_weather" value=")rawHTML";
  html += ha_config.ha_enitty_weather_name;
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

String HttpServer::buildFooter()
{
  String html;
  html += R"rawHTML(
        <div class="footer">
            <p>&copy; 2026 Inżynier Domu. Wszystkie prawa zastrzeżone.</p>
            <div class="footer-icons">
                <a href="https://github.com/InzynierDomu/E-ink-smart-alarm-clock" target="_blank" title="GitHub" rel="noopener">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                        <path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v 3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z"/>
                    </svg>
                </a>
                <a href="https://buycoffee.to/inzynier-domu" target="_blank" title="Postaw kawę" rel="noopener">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                        <path d="M20 3H4v10c0 2.21 1.79 4 4 4h6c2.21 0 4-1.79 4-4v-3h2c1.11 0 2-.9 2-2V5c0-1.11-.89-2-2-2zm0 5h-2V5h2v3zM4 19h16v2H4z"/>
                    </svg>
                </a>
                <a href="https://www.inzynierdomu.pl/" target="_blank" title="Blog" rel="noopener">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                        <path d="M19 2H5a3 3 0 00-3 3v14a3 3 0 003 3h14a3 3 0 003-3V5a3 3 0 00-3-3zm0 16H5V5h14v13zm-7-9h-4v2h4V9zm6 0h-4v2h4V9zm0 4h-6v2h6v-2z"/>
                    </svg>
                </a>
                <a href="https://www.youtube.com/c/InzynierDomu?sub_confirmation=1" target="_blank" title="YouTube" rel="noopener">
                    <svg width="20" height="20" viewBox="0 0 24 24" fill="currentColor">
                        <path d="M23.498 6.186a3.016 3.016 0 0 0-2.122-2.136C19.505 3.545 12 3.545 12 3.545s-7.505 0-9.377.505A3.017 3.017 0 0 0 .502 6.186C0 8.07 0 12 0 12s0 3.93.502 5.814a3.016 3.016 0 0 0 2.122 2.136c1.871.505 9.376.505 9.376.505s7.505 0 9.377-.505a3.015 3.015 0 0 0 2.122-2.136C24 15.93 24 12 24 12s0-3.93-.502-5.814zM9.545 15.568V8.432L15.818 12l-6.273 3.568z"/>
                    </svg>
                </a>
            </div>
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
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #0f172a 0%, #1e293b 100%);
            color: #e2e8f0;
            min-height: 100vh;
            padding: 20px;
        }

        .container {
            max-width: 900px;
            margin: 0 auto;
        }

        .header {
            text-align: center;
            margin-bottom: 40px;
            padding-bottom: 30px;
            border-bottom: 2px solid #334155;
        }

        .header-logo {
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 15px;
            margin-bottom: 15px;
        }

        .header-logo svg {
            width: 60px;
            height: 60px;
            filter: drop-shadow(0 0 10px rgba(96, 165, 250, 0.5));
        }

        .header h1 {
            font-size: 2.5em;
            background: linear-gradient(135deg, #60a5fa, #a78bfa);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
            margin-bottom: 5px;
        }

        .header p {
            color: #94a3b8;
            font-size: 0.95em;
        }

        .section {
            background: rgba(30, 41, 59, 0.6);
            border: 1px solid #334155;
            border-radius: 12px;
            padding: 25px;
            margin-bottom: 25px;
            backdrop-filter: blur(10px);
            transition: all 0.3s ease;
        }

        .section:hover {
            border-color: #475569;
            background: rgba(30, 41, 59, 0.8);
        }

        .section-title {
            font-size: 1.3em;
            font-weight: 600;
            color: #60a5fa;
            margin-bottom: 20px;
            padding-bottom: 10px;
            border-bottom: 2px solid #334155;
        }

        .form-row {
            display: grid;
            grid-template-columns: 180px 1fr;
            gap: 15px;
            margin-bottom: 18px;
            align-items: center;
        }

        .form-row:last-child {
            margin-bottom: 0;
        }

        .form-label {
            font-weight: 500;
            color: #cbd5e1;
            font-size: 0.95em;
        }

        input[type="text"],
        input[type="password"],
        input[type="email"],
        input[type="number"],
        input[type="url"],
        select,
        textarea {
            background: rgba(15, 23, 42, 0.8);
            border: 1px solid #475569;
            border-radius: 8px;
            padding: 10px 12px;
            color: #e2e8f0;
            font-size: 0.95em;
            transition: all 0.3s ease;
            width: 100%;
            font-family: inherit;
        }

        input[type="text"]:focus,
        input[type="password"]:focus,
        input[type="email"]:focus,
        input[type="number"]:focus,
        input[type="url"]:focus,
        select:focus,
        textarea:focus {
            outline: none;
            border-color: #60a5fa;
            background: rgba(15, 23, 42, 0.95);
            box-shadow: 0 0 0 3px rgba(96, 165, 250, 0.1);
        }

        input[type="checkbox"] {
            width: 20px;
            height: 20px;
            cursor: pointer;
            accent-color: #60a5fa;
        }

        .button-group {
            display: flex;
            gap: 12px;
            margin-top: 30px;
            justify-content: center;
        }

        button {
            background: linear-gradient(135deg, #60a5fa, #3b82f6);
            color: white;
            border: none;
            padding: 12px 32px;
            border-radius: 8px;
            font-size: 1em;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            box-shadow: 0 4px 15px rgba(96, 165, 250, 0.3);
            font-family: inherit;
        }

        button:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(96, 165, 250, 0.4);
        }

        button:active {
            transform: translateY(0);
        }

        button[type="reset"] {
            background: linear-gradient(135deg, #64748b, #475569);
            box-shadow: 0 4px 15px rgba(100, 116, 139, 0.2);
        }

        button[type="reset"]:hover {
            box-shadow: 0 6px 20px rgba(100, 116, 139, 0.3);
        }

        .logo-image {
            width: 50px;
            height: 50px;
            border-radius: 12px;
            filter: drop-shadow(0 0 10px rgba(96, 165, 250, 0.5));
            object-fit: cover;
        }

        .footer {
            text-align: center;
            margin-top: 50px;
            padding-top: 30px;
            border-top: 2px solid #334155;
            color: #94a3b8;
        }

        .footer p {
            margin-bottom: 15px;
            font-size: 0.95em;
        }

        .footer a {
            color: #60a5fa;
            text-decoration: none;
            transition: color 0.3s ease;
            font-weight: 500;
        }

        .footer a:hover {
            color: #a78bfa;
            text-decoration: underline;
        }

        .footer-icons {
            display: flex;
            justify-content: center;
            gap: 20px;
            margin-top: 15px;
        }

        .footer-icons a {
            display: inline-flex;
            align-items: center;
            justify-content: center;
            width: 40px;
            height: 40px;
            background: rgba(96, 165, 250, 0.1);
            border-radius: 50%;
            transition: all 0.3s ease;
            color: #60a5fa;
        }

        .footer-icons a:hover {
            background: rgba(96, 165, 250, 0.2);
            transform: scale(1.1);
            color: #a78bfa;
        }

        @media (max-width: 600px) {
            .form-row {
                grid-template-columns: 1fr;
                gap: 8px;
            }

            .header h1 {
                font-size: 1.8em;
            }

            .button-group {
                flex-direction: column;
            }

            button {
                width: 100%;
            }
        }
    </style>
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
  page += buildTimezoneSection();
  page += buildWeatherSection();
  page += buildGoogleCalendarSection();
  page += buildHaSection();
  page += buildAudioSection();

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
  int tz_hours = server_.arg("timezone_hours").toInt();
  int tz_seconds = tz_hours * 3600;

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
  String new_ha_entity = server_.arg("ha_entity_weather");
  bool new_weather_from_ha = server_.arg("weather_from_ha").toInt();

  doc["ssid"] = new_ssid;
  doc["pass"] = new_pass;
  doc["timezone"] = tz_seconds;

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
  doc["HA_weather_entity_name"] = new_ha_entity;
  doc["weather_from_HA"] = new_weather_from_ha;
}
