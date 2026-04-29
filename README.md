# E-ink smart alarm clock

![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/InzynierDomu/E-ink-smart-alarm-clock/main.yml?logo=github&style=flat-square)
![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/InzynierDomu/E-ink-smart-alarm-clock?style=flat-square)
<a href="https://discord.gg/KmW6mHdg">![Discord](https://img.shields.io/discord/815929748882587688?logo=discord&logoColor=green&style=flat-square)</a>
![GitHub](https://img.shields.io/github/license/InzynierDomu/PhECMeter?style=flat-square)

## Description

A minimalist clock and alarm that displays the daily weather forecast and calendar events. Thanks to its e-ink screen, it emits no light, making it perfectly suited for the bedroom and gentle on your eyes at night.

## Features

Video about device [video](https://youtu.be/p92zGk0prb8)

- Clock and calendar
- Alarm
- Weather forecast
- Google Calendar events list (today only, including recurring events)
- Home Assistant integration (optional)

![photo](https://www.inzynierdomu.pl/wp-content/uploads/2026/03/Zegarek-eink-scaled.jpg)

<div align="center">
<h2>Support</h2>

<p>If any of my projects have helped you in your work, studies, or simply made your day better, you can buy me a coffee. <a href="https://buycoffee.to/inzynier-domu" target="_blank"><img src="https://buycoffee.to/img/share-button-primary.png" style="width: 195px; height: 51px" alt="Postaw mi kawę na buycoffee.to"></a></p>
</div>

## Required environment

- **Board**: [CrowPanel ESP32 5.79inch E-paper](https://www.elecrow.com/crowpanel-esp32-5-79-e-paper-hmi-display-with-272-792-resolution-black-white-color-driven-by-spi-interface.html?idd=6)
- **Platform**: PlatformIO [video](https://platformio.org/)
- **Framework**: Arduino

## Hardware

### Parts

- [CrowPanel ESP32 5.79inch E-paper](https://www.elecrow.com/crowpanel-esp32-5-79-e-paper-hmi-display-with-272-792-resolution-black-white-color-driven-by-spi-interface.html)
- DS1307
- MAX98357
- Speaker
- Touch button with LED backlight

### Schematic

![schem](https://github.com/InzynierDomu/E-ink-smart-alarm-clock/blob/main/schem.png)

## Installation

### SD Card Setup

1. Format the SD card as FAT32.
2. Copy the alarm sound file named `ringtone.wav` to the root of the SD card.
3. Insert the SD card into the device — configuration is done via the web interface on first boot.

### First Boot & Configuration

1. Power the device via USB-C (5V, e.g. phone charger).
2. On first boot the device starts in **AP (Access Point) mode**.
3. Connect to the Wi-Fi network **`AlarmClock`** (password: `inzynier_domu`) from your phone or computer.
4. Open a browser and go to **`http://192.168.4.1`**.
5. Fill in the configuration form (see sections below) and click **Save**.
6. The device will restart and connect to your home Wi-Fi.

To reset the configuration at any time, press the button **5 times within 10 seconds** — the device will clear the config and restart in AP mode.

### Configuration fields

**Wi-Fi**
- SSID and password of your home network.

**Weather (OpenWeatherMap)**
- Register at [openweathermap.org](https://openweathermap.org/) and get a free API key.
- Enter your latitude and longitude (right-click your location on Google Maps to copy coordinates).

**Google Calendar (iCal)**

The device fetches today's events directly from Google Calendar using a private iCal URL. Recurring events are fully supported.

- Open [Google Calendar](https://calendar.google.com) → Settings → select your calendar → scroll to **Integrate calendar** → copy the **Secret address in iCal format**.
- Paste it into the **Calendar URL (events)** field.
- Optionally paste a second iCal URL into the **Alarm calendar URL** field — any event in that calendar will set the alarm to its start time.

> Calendar, alarm and weather data refresh automatically every ~11 minutes. The first fetch happens about one minute after boot.

**Home Assistant** *(optional)*
- Enter your HA host IP, port (`8123`), and a long-lived access token (HA → Profile → Long-Lived Access Tokens).
- Provide the weather entity name (e.g. `weather.home`) to pull temperature from HA instead of OpenWeatherMap.

## Usage

| Action | Effect |
|--------|--------|
| Short press | Toggle alarm on / off (mute/unmute) |
| Press while ringing | Dismiss alarm |
| 5 quick presses in 10 seconds | Reset config, restart in AP mode |

> After each press wait a moment — the mute/unmute change is visible on screen. Pressing too fast may be interpreted as the reset sequence.

## Buy a ready-made device

Don't want to build it yourself? You can order a ready-made device directly from me:

👉 **[inzynierdomu.pl/zegarek-przyjemny-dla-oka](https://www.inzynierdomu.pl/zegarek-przyjemny-dla-oka/)**

## Thanks

Repositories I used:
- https://github.com/kristiantm/eink-family-calendar-esp32
- https://github.com/MakersFunDuck/5.79-inch-E-ink-dashboard-with-LVGL-9
