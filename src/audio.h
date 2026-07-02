/**
 * @file audio.h
 * @brief Interface of the Audio class handling sound playback over I2S from an SD card.
 */

#pragma once
#include <Arduino.h>

struct Audio_config
{
  uint16_t sample_rate = 44100;
  uint8_t volume = 65;
};

enum class Audio_error : uint8_t
{
  none = 0,
  file_not_found,
  mutex_timeout,
};

class Audio
{
  public:
  Audio() {}
  void setup();
  void play_audio();
  void set_config(Audio_config& config);
  void get_config(Audio_config& config);
  void stop();
  void start();
  void wait_until_idle();
  Audio_error take_last_error();

  private:
  volatile bool stop_requested = true;
  volatile Audio_error last_error = Audio_error::none;
  const uint16_t audio_buffer_size = 512;
  Audio_config config;
};
