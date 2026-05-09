/**
 * @file RTClib.h
 * @brief Minimal DateTime/TimeSpan stub for native unit tests.
 */

#pragma once
#include <cstdint>

class DateTime
{
public:
  DateTime() = default;
  DateTime(uint16_t, uint8_t, uint8_t, uint8_t h = 0, uint8_t m = 0, uint8_t s = 0)
  : _h(h), _m(m), _s(s) {}

  uint8_t hour()   const { return _h; }
  uint8_t minute() const { return _m; }
  uint8_t second() const { return _s; }

private:
  uint8_t _h = 0, _m = 0, _s = 0;
};

class TimeSpan
{
public:
  TimeSpan(int32_t seconds = 0) : _s(seconds) {}
  TimeSpan(int days, int hours, int minutes, int seconds)
  : _s((int32_t)days * 86400L + hours * 3600L + minutes * 60L + seconds) {}

private:
  int32_t _s;
};
