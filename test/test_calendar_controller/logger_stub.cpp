#include "logger.h"

String Logger::_path = "";
size_t Logger::_max_bytes = 0;
bool Logger::_muted = false;

void Logger::setup(const String&, size_t) {}
void Logger::info(const String&, const String&) {}
void Logger::warn(const String&, const String&) {}
void Logger::error(const String&, const String&) {}
const String& Logger::path() { return _path; }
void Logger::mute() { _muted = true; }
void Logger::unmute() { _muted = false; }
bool Logger::is_muted() { return _muted; }
void Logger::write(LogLevel, const String&, const String&) {}
void Logger::rotate_if_needed() {}
String Logger::timestamp() { return String(); }
