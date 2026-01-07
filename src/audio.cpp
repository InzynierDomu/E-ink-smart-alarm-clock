#include "audio.h"

#include "config.h"

#include <SD.h>
#include <driver/i2s.h>

void Audio::setup()
{
  Serial.println("audio cofing start");
  i2s_config_t i2s_config = {.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
                             .sample_rate = config.sample_rate,
                             .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                             .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
                             .communication_format = I2S_COMM_FORMAT_I2S,
                             .intr_alloc_flags = 0,
                             .dma_buf_count = 8,
                             .dma_buf_len = audio_buffer_size,
                             .use_apll = false};
  i2s_pin_config_t pin_config = {.bck_io_num = config::speaker_bck_pin,
                                 .ws_io_num = config::speaker_ws_pin,
                                 .data_out_num = config::speaker_dout_pin,
                                 .data_in_num = I2S_PIN_NO_CHANGE};
  esp_err_t err = i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
  Serial.println(esp_err_to_name(err));
  err = i2s_set_pin(I2S_NUM_1, &pin_config);
  Serial.println(esp_err_to_name(err));
  Serial.println("audio cofing end");
}

void Audio::play_audio()
{
  File audioFile;
  Serial.println("playing audio...");
  audioFile = SD.open(config::audio_path, FILE_READ);
  if (!audioFile)
  {
    Serial.println("error with audio file");
    return;
  }

  i2s_start(I2S_NUM_1);
  size_t bytesRead;
  int16_t buffer[audio_buffer_size];

  float volFactor = config.volume / 100.0f;

  while (audioFile.available())
  {
    bytesRead = audioFile.read((uint8_t*)buffer, audio_buffer_size * sizeof(int16_t));

    size_t samples = bytesRead / sizeof(int16_t);
    for (size_t i = 0; i < samples; i++)
    {
      int32_t sample = buffer[i];
      sample = (int32_t)(sample * volFactor);

      buffer[i] = (int16_t)sample;
    }

    i2s_write(I2S_NUM_1, buffer, bytesRead, &bytesRead, portMAX_DELAY);
  }

  audioFile.close();

  i2s_stop(I2S_NUM_1);

  Serial.println("playing end.");
}

void Audio::set_config(Audio_config& _config)
{
  config = _config;
  if (config.volume > 100)
  {
    config.volume = 100;
  }
}

void Audio::get_config(Audio_config& _config)
{
  _config = config;
}