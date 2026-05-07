#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

enum class LogLevel
{
  INFO,
  WARN,
  ERROR
};

class Logger
{
  public:
  static void setup(const String& path = "/logs.txt", size_t max_kb = 50);
  static void info(const String& tag, const String& msg);
  static void warn(const String& tag, const String& msg);
  static void error(const String& tag, const String& msg);
  static const String& path();

  private:
  static void write(LogLevel level, const String& tag, const String& msg);
  static void rotate_if_needed();
  static void flush_buffer();
  static String timestamp();

  static String _path;
  static size_t _max_bytes;

  static constexpr uint8_t LOG_BUF_SIZE = 8;
  static String _buf[LOG_BUF_SIZE];
  static uint8_t _buf_count;
};
