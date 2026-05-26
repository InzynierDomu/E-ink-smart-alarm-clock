/**
 * @file logger.cpp
 * @brief Logger implementation — writing messages to an SD file with rotation and timestamps.
 */

#include "logger.h"

#include <SD.h>
#include <time.h>

extern SemaphoreHandle_t sd_mutex;

String Logger::_path;
size_t Logger::_max_bytes;
SemaphoreHandle_t Logger::_sd_mutex = nullptr;

// Exposed so the audio task can hold the mutex during playback.
SemaphoreHandle_t g_sd_mutex = nullptr;

/**
 * @brief Initializes the logger — sets the file path, maximum file size, and creates the SD mutex.
 * @param path Path to the log file on the SD card.
 * @param max_kb Maximum log file size in kilobytes.
 */
void Logger::setup(const String& path, size_t max_kb)
{
  _path      = path;
  _max_bytes = max_kb * 1024;
  if (!_sd_mutex)
  {
    _sd_mutex  = xSemaphoreCreateMutex();
    g_sd_mutex = _sd_mutex;
  }
}

/**
 * @brief Returns the path to the log file.
 * @return Reference to the string containing the log file path.
 */
const String& Logger::path()
{
  return _path;
}

/**
 * @brief Writes an INFO level message to the log.
 */
void Logger::info(const String& tag, const String& msg)
{
  write(LogLevel::INFO, tag, msg);
}

/**
 * @brief Writes a WARN level message to the log.
 */
void Logger::warn(const String& tag, const String& msg)
{
  write(LogLevel::WARN, tag, msg);
}

/**
 * @brief Writes an ERROR level message to the log.
 */
void Logger::error(const String& tag, const String& msg)
{
  write(LogLevel::ERROR, tag, msg);
}

/**
 * @brief Generates a timestamp in "YYYY-MM-DD HH:MM:SS" format, or uptime after a reset.
 */
String Logger::timestamp()
{
  time_t now = time(nullptr);
  if (now < 1700000000UL)
  {
    char buf[16];
    unsigned long s = millis() / 1000;
    snprintf(buf, sizeof(buf), "+%lus", s);
    return String(buf);
  }
  struct tm t;
  localtime_r(&now, &t);
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec);
  return String(buf);
}

/**
 * @brief Deletes the log file if it exceeds the maximum allowed size.
 */
void Logger::rotate_if_needed()
{
  if (!SD.exists(_path))
    return;
  File f = SD.open(_path, "r");
  if (!f)
    return;
  size_t size = f.size();
  f.close();
  if (size > _max_bytes)
    SD.remove(_path);
}

/**
 * @brief Writes a formatted log entry to the SD file.
 *        Acquires the SD mutex with a short timeout — drops the write if audio holds the bus.
 */
void Logger::write(LogLevel level, const String& tag, const String& msg)
{
  const char* lvl_str = level == LogLevel::ERROR ? "ERROR" : level == LogLevel::WARN ? "WARN" : "INFO";
  String line = "[" + timestamp() + "] " + lvl_str + " [" + tag + "] " + msg + "\n";

  Serial.print("[LOG] ");
  Serial.print(line);

  if (_path.isEmpty())
    return;

  if (xSemaphoreTake(_sd_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
    return;

  rotate_if_needed();

  File f = SD.open(_path, FILE_APPEND);
  if (f)
  {
    f.print(line);
    f.close();
  }

  xSemaphoreGive(_sd_mutex);
}
