#include "logger.h"

String Logger::_path = "";
size_t Logger::_max_bytes = 0;
SemaphoreHandle_t Logger::_sd_mutex = nullptr;

void Logger::setup(const String&, size_t) {}
void Logger::info(const String&, const String&) {}
void Logger::warn(const String&, const String&) {}
void Logger::error(const String&, const String&) {}
const String& Logger::path() { return _path; }
void Logger::write(LogLevel, const String&, const String&) {}
void Logger::rotate_if_needed() {}
String Logger::timestamp() { return String(); }
