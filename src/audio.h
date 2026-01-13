#pragma once
#include <Arduino.h>

struct Audio_config
{
  uint16_t sample_rate;
  uint8_t volume;
};

class Audio
{
  public:
  Audio() {}
  void setup();
  void play_audio();
  void set_config(Audio_config& config);

  void get_config(Audio_config& config);

  private:
  const uint16_t audio_buffer_size = 512;
  Audio_config config;
};
