# E-ink smart alarm clock

![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/InzynierDomu/E-ink-smart-alarm-clock/main.yml?logo=github&style=flat-square)
![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/InzynierDomu/E-ink-smart-alarm-clock?style=flat-square)
<a href="https://discord.gg/KmW6mHdg">![Discord](https://img.shields.io/discord/815929748882587688?logo=discord&logoColor=green&style=flat-square)</a>
![GitHub](https://img.shields.io/github/license/InzynierDomu/PhECMeter?style=flat-square)

## Description

A minimalist clock and alarm that displays the daily weather forecast and calendar events. Thanks to its e-ink screen, it emits no light, making it perfectly suited for the bedroom and gentle on your eyes at night.

## Features

- Clock and calendar
- Alarm
- weather forecast
- Google calendar events list

![photo](https://github.com/InzynierDomu/E-ink-smart-alarm-clock/blob/main/photo.JPG)

<div align="center">
<h2>Support</h2>

<p>If any of my projects have helped you in your work, studies, or simply made your day better, you can buy me a coffee. <a href="https://buycoffee.to/inzynier-domu" target="_blank"><img src="https://buycoffee.to/img/share-button-primary.png" style="width: 195px; height: 51px" alt="Postaw mi kawę na buycoffee.to"></a></p>
</div>

## Required environment

- **Board**: [CrowPanel ESP32 5.79inch E-paper](https://www.elecrow.com/crowpanel-esp32-5-79-e-paper-hmi-display-with-272-792-resolution-black-white-color-driven-by-spi-interface.html)
- **Platform**: PlatformIO [video](https://platformio.org/) 
- **Framework**: Arduino 

## Installation

### Parts

- [CrowPanel ESP32 5.79inch E-paper](https://www.elecrow.com/crowpanel-esp32-5-79-e-paper-hmi-display-with-272-792-resolution-black-white-color-driven-by-spi-interface.html)
- DS1307
- MAX98357
- Speaker
- Touch button with LED backlight 

### Schem

![schem](https://github.com/InzynierDomu/E-ink-smart-alarm-clock/blob/main/schem.png)

### SD Card Setup

1. Format the SD card as FAT32.
2. Configuration file - fulfill [config.json](https://github.com/InzynierDomu/E-ink-smart-alarm-clock/blob/main/config.json) and copy to SD card
   - you need to register at [openweathermap](https://openweathermap.org/) and obtain an API key
   - enter your latitude and longitude where you want to read the weather
   - you need a WiFi connection (SSID and password) to synchronize the clock
   - sample_rate should be changed if the ringtone you uploaded sounds slowed down or sped up, then check the sample rate of this sound
3. Alarm - copy the selected sound with the name ringtone.wav

### Google calendar script

1. Create your own Google Apps Script at [script.new](https://script.new/)
2. Copy the script content directly in [google_apps_script_json.js](https://github.com/InzynierDomu/E-ink-smart-alarm-clock/blob/main/google_apps_script_json.js)
3. After creating the script, copy the unique link (webhook/API URL) provided by Google and enter it in the configuration file on the SD card in the google_script_url field.

### Filtering calendar

1. To filter the alarm and events displayed on the watch, you need to provide the IDs of the calendars you want to filter in the configuration file
2. You can get the ID by running the google script - implementation test

## Usage

1. To add an alarm, set an event in Google Calendar in the appropriate calendar provided during configuration.
2. to change the alarm ON/OFF press the button
3. To turn off the alarm, press the button while it is ringing

## Thanks

Repositories I used:
- https://github.com/kristiantm/eink-family-calendar-esp32
- https://github.com/MakersFunDuck/5.79-inch-E-ink-dashboard-with-LVGL-9