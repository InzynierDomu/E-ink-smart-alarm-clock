/**
 * @file audio.cpp
 * @brief Implementation of WAV file playback over the I2S interface on ESP32.
 */

#include "audio.h"

#include "config.h"

#include <SD.h>
#include <driver/i2s.h>
#include <freertos/semphr.h>

extern SemaphoreHandle_t g_sd_mutex;

/**
 * @brief Initializes the I2S driver with the current audio configuration.
 */
void Audio::setup()
{
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
  i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_1, &pin_config);
}

/**
 * @brief Plays an audio file from the SD card over I2S with volume scaling applied.
 */
void Audio::play_audio()
{
  // Hold the SD mutex for the entire playback so the logger cannot access SD concurrently.
  // Logger::write() uses a 100 ms timeout and drops writes while we hold the mutex.
  xSemaphoreTake(g_sd_mutex, portMAX_DELAY);

  File audioFile = SD.open(config::audio_path, FILE_READ);
  if (!audioFile)
  {
    xSemaphoreGive(g_sd_mutex);
    return;
  }

  i2s_start(I2S_NUM_1);
  size_t bytesRead;
  int16_t buffer[audio_buffer_size];

  float volFactor = config.volume / 100.0f;

  while (audioFile.available() && !stop_requested)
  {
    bytesRead = audioFile.read((uint8_t*)buffer, audio_buffer_size * sizeof(int16_t));

    size_t samples = bytesRead / sizeof(int16_t);
    for (size_t i = 0; i < samples; i++)
    {
      int32_t sample = buffer[i];
      sample = (int32_t)(sample * volFactor);
      buffer[i] = (int16_t)sample;
    }

    i2s_write(I2S_NUM_1, buffer, bytesRead, &bytesRead, pdMS_TO_TICKS(500));
    vTaskDelay(1); // oddaj CPU żeby IDLE0 mógł zasilić WDT
  }

  audioFile.close();
  i2s_stop(I2S_NUM_1);
  xSemaphoreGive(g_sd_mutex);
}

/**
 * @brief Sets the audio configuration — sample rate and volume (range 1–100).
 * @param _config Reference to the structure with the new configuration.
 */
void Audio::set_config(Audio_config& _config)
{
  if (_config.sample_rate > 0)
    config.sample_rate = _config.sample_rate;
  if (_config.volume > 0)
    config.volume = (_config.volume > 100) ? 100 : _config.volume;
}

/**
 * @brief Copies the current audio configuration into the provided structure.
 * @param _config Reference to the structure that will receive the configuration.
 */
void Audio::get_config(Audio_config& _config)
{
  _config = config;
}

/**
 * @brief Requests audio playback to stop.
 */
void Audio::stop()
{
  stop_requested = true;
}

/**
 * @brief Allows audio playback to proceed by clearing the stop flag.
 */
void Audio::start()
{
  stop_requested = false;
}
