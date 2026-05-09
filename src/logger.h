/**
 * @file logger.h
 * @brief Logger interface for writing messages to an SD card with INFO/WARN/ERROR levels.
 */

#pragma once
#include <Arduino.h>

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
  static void mute();
  static void unmute();
  static bool is_muted();

  private:
  static void write(LogLevel level, const String& tag, const String& msg);
  static void rotate_if_needed();
  static String timestamp();

  static String _path;
  static size_t _max_bytes;
  static bool _muted;
};
