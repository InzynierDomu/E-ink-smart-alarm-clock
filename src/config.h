#pragma once

#include <Arduino.h>

namespace config
{
constexpr uint8_t sd_cs_pin = 10;
constexpr uint8_t sd_power_pin = 42;
constexpr uint8_t screen_power_pin = 7;
constexpr uint8_t alarm_enable_button_pin = 2;
constexpr uint8_t speaker_bck_pin = 19;
constexpr uint8_t speaker_ws_pin = 20;
constexpr uint8_t speaker_dout_pin = 3;
constexpr uint8_t sda_pin = 21;
constexpr uint8_t scl_pin = 38;
constexpr uint8_t btn_pin = 8;
constexpr uint8_t led_pin = 14;

constexpr uint16_t screen_width = 792;
constexpr uint16_t screen_height = 272;
constexpr uint16_t lv_buffer = ((screen_width * screen_height / 8) + 8);

const String config_path = "/config.json";

const String audio_path = "/ringtone.wav";

constexpr unsigned int local_port = 2390; // local port to listen for UDP packets
constexpr char time_server[] = "tempus1.gum.gov.pl"; // extenral NTP server
constexpr int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
} // namespace config