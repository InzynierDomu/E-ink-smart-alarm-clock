#include "logger.h"

#include <SD.h>
#include <time.h>

String Logger::_path;
size_t Logger::_max_bytes;

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
  // time() returns seconds since epoch; before NTP sync it is near 0
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
  if (size > _max_bytes)
  {
    SD.remove(_path);
    Serial.println("[LOG] rotated (file too large)");
  }
}

void Logger::write(LogLevel level, const String& tag, const String& msg)
{
  const char* lvl_str = level == LogLevel::ERROR ? "ERROR" : level == LogLevel::WARN ? "WARN" : "INFO";

  String line = "[" + timestamp() + "] " + lvl_str + " [" + tag + "] " + msg + "\n";

  Serial.print("[LOG] ");
  Serial.print(line);

  if (_path.isEmpty())
    return;

  rotate_if_needed();

  File f = SD.open(_path, FILE_APPEND);
  if (!f)
  {
    Serial.println("[LOG] cannot open log file");
    return;
  }
  f.print(line);
  f.close();
}
