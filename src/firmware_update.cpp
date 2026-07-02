/**
 * @file firmware_update.cpp
 * @brief Implementation of ESP32 firmware update from a firmware.bin file on the SD card.
 */

#include "firmware_update.h"

#include <Arduino.h>
#include <SD.h>
#include <Update.h>


static const char* kUpdateFilePath = "/firmware.bin";

/**
 * @brief Checks for a firmware.bin file on the SD card and performs an OTA update if the file is present.
 * @return true if the update succeeded and the device restarted, false in all other cases.
 */
bool checkAndPerformUpdateFromSD()
{
  if (SD.cardType() == CARD_NONE)
  {
    Serial.println("SD not available");
    return false;
  }

  if (!SD.exists(kUpdateFilePath))
  {
    return false;
  }

  File updateBin = SD.open(kUpdateFilePath, FILE_READ);
  if (!updateBin)
  {
    Serial.println("Failed to open firmware.bin");
    return false;
  }

  size_t updateSize = updateBin.size();
  if (updateSize == 0)
  {
    Serial.println("Empty firmware.bin");
    updateBin.close();
    return false;
  }

  Serial.printf("Starting update, size=%u bytes\n", (unsigned)updateSize);

  if (!Update.begin(updateSize))
  {
    Serial.printf("Update.begin failed, error=%s\n", Update.errorString());
    updateBin.close();
    return false;
  }

  size_t written = Update.writeStream(updateBin);
  updateBin.close();

  if (written != updateSize)
  {
    Serial.printf("Written %u/%u bytes\n", (unsigned)written, (unsigned)updateSize);
    Update.abort();
    return false;
  }

  if (!Update.end())
  {
    Serial.printf("Update.end failed, error=%s\n", Update.errorString());
    return false;
  }

  if (!Update.isFinished())
  {
    Serial.println("Update not finished");
    return false;
  }

  SD.remove(kUpdateFilePath);
  Serial.println("Update successful, restarting...");
  delay(500);
  ESP.restart();
  return true;
}