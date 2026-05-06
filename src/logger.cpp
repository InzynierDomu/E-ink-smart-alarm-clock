#include "logger.h"

#include <SD.h>
#include <time.h>

extern SemaphoreHandle_t sd_mutex;

String Logger::_path;
size_t Logger::_max_bytes;
String Logger::_buf[Logger::LOG_BUF_SIZE];
uint8_t Logger::_buf_count = 0;

void Logger::setup(const String& path, size_t max_kb)
{
  _path      = path;
  _max_bytes = max_kb * 1024;
}

const String& Logger::path()
{
  return _path;
}

void Logger::info(const String& tag, const String& msg)
{
  write(LogLevel::INFO, tag, msg);
}

void Logger::warn(const String& tag, const String& msg)
{
  write(LogLevel::WARN, tag, msg);
}

void Logger::error(const String& tag, const String& msg)
{
  write(LogLevel::ERROR, tag, msg);
}

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

void Logger::rotate_if_needed()
{
  if (!SD.exists(_path))
    return;
  File f = SD.open(_path, "r");
  if (!f)
    return;
  size_t size = f.size();
  f.close();
  if (size <= _max_bytes)
    return;

  time_t now = time(nullptr);
  char dated[28];
  if (now < 1700000000UL)
  {
    snprintf(dated, sizeof(dated), "/logs_unknown.txt");
  }
  else
  {
    struct tm t;
    localtime_r(&now, &t);
    snprintf(dated, sizeof(dated), "/logs_%04d%02d%02d.txt",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  }
  if (SD.exists(dated))
    SD.remove(dated);
  SD.rename(_path.c_str(), dated);
  Serial.printf("[LOG] rotated to %s\n", dated);
}

void Logger::flush_buffer()
{
  for (uint8_t i = 0; i < _buf_count; i++)
  {
    rotate_if_needed();
    File f = SD.open(_path, FILE_APPEND);
    if (f)
    {
      f.print(_buf[i]);
      f.close();
    }
  }
  _buf_count = 0;
}

void Logger::write(LogLevel level, const String& tag, const String& msg)
{
  const char* lvl_str = level == LogLevel::ERROR ? "ERROR" : level == LogLevel::WARN ? "WARN" : "INFO";
  String line = "[" + timestamp() + "] " + lvl_str + " [" + tag + "] " + msg + "\n";

  Serial.print("[LOG] ");
  Serial.print(line);

  if (_path.isEmpty())
    return;

  bool got_mutex = (sd_mutex == nullptr) || (xSemaphoreTake(sd_mutex, 0) == pdTRUE);

  if (got_mutex)
  {
    flush_buffer();
    rotate_if_needed();
    File f = SD.open(_path, FILE_APPEND);
    if (f)
    {
      f.print(line);
      f.close();
    }
    if (sd_mutex != nullptr)
      xSemaphoreGive(sd_mutex);
  }
  else
  {
    if (_buf_count < LOG_BUF_SIZE)
      _buf[_buf_count++] = line;
    else
      Serial.println("[LOG] buffer full, entry dropped");
  }
}
