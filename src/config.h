/**
 * @file config.h
 * @brief Project configuration constants — pin assignments, screen dimensions, file paths, and network parameters.
 */

#pragma once
#include <Arduino.h>
namespace config
{
constexpr uint8_t sd_cs_pin = 10; ///< SD card chip select pin.
constexpr uint8_t sd_power_pin = 42; ///< SD card power enable pin.
constexpr uint8_t screen_power_pin = 7; ///< E-ink screen power enable pin.
constexpr uint8_t alarm_enable_button_pin = 2; ///< Auxiliary alarm enable button pin.
constexpr uint8_t speaker_bck_pin = 19; ///< I2S bit clock pin for the speaker.
constexpr uint8_t speaker_ws_pin = 20; ///< I2S word select pin for the speaker.
constexpr uint8_t speaker_dout_pin = 3; ///< I2S data output pin for the speaker.
constexpr uint8_t sda_pin = 21; ///< I2C SDA pin (RTC).
constexpr uint8_t scl_pin = 38; ///< I2C SCL pin (RTC).
constexpr uint8_t btn_pin = 8; ///< Main user button pin.
constexpr uint8_t led_pin = 14; ///< Status LED pin.

constexpr uint16_t screen_width = 792; ///< E-ink display width in pixels.
constexpr uint16_t screen_height = 272; ///< E-ink display height in pixels.
constexpr uint16_t lv_buffer = ((screen_width * screen_height / 8) + 8); ///< LVGL draw buffer size in bytes.

const String config_path = "/config.json"; ///< Path to the configuration file on the SD card.

const String audio_path = "/ringtone.wav"; ///< Path to the alarm audio file on the SD card.

constexpr unsigned int local_port = 2390; ///< Local UDP port used for NTP requests.
constexpr char time_server[] = "tempus1.gum.gov.pl"; ///< NTP server address.
constexpr int NTP_PACKET_SIZE = 48; ///< Size of an NTP packet in bytes.

constexpr unsigned long btn_debounce_ms = 75; ///< Button debounce time in milliseconds.
constexpr uint8_t reset_press_count_threshold = 5; ///< Number of presses required to trigger a config reset.
constexpr unsigned long reset_window_ms = 10000; ///< Time window in ms within which reset presses must occur.

const String version = "1.2.15"; ///< Firmware version string.

constexpr uint8_t day_part_morning_from = 6; ///< Hour from which morning starts; before this hour the night part is shown.
constexpr uint8_t day_part_morning_until = 10; ///< Hour (exclusive) until which the morning weather part is shown.
constexpr uint8_t day_part_afternoon_until = 16; ///< Hour (exclusive) until which the afternoon weather part is shown.
                                                 ///< After this hour the evening part is shown.
} // namespace config