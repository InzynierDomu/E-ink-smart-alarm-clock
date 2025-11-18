#pragma once
#include <Arduino.h>

class Audio
{
  public:
  Audio() {}
  void setup();
  void play_audio();
  void set_sample_rate(uint16_t sample_rate);

  private:
  const uint16_t audio_buffer_size = 512;
  uint16_t sample_rate = 16000;
};
