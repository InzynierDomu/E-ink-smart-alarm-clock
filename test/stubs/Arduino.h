#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ostream>

using std::round;

class String
{
  public:
  String() {}
  String(const char* s) : _s(s ? s : "") {}
  String(const std::string& s) : _s(s) {}
  String(int v) : _s(std::to_string(v)) {}
  String(unsigned int v) : _s(std::to_string(v)) {}
  String(long v) : _s(std::to_string(v)) {}
  String(unsigned long v) : _s(std::to_string(v)) {}
  String(uint8_t v) : _s(std::to_string((int)v)) {}
  String(int8_t v) : _s(std::to_string((int)v)) {}

  String(float v, int decimals = 2)
  {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << v;
    _s = oss.str();
  }

  String substring(int start, int end) const
  {
    if (start >= (int)_s.size())
      return String();
    return String(_s.substr(start, end - start));
  }

  int toInt() const
  {
    try
    {
      return std::stoi(_s);
    }
    catch (...)
    {
      return 0;
    }
  }

  bool isEmpty() const { return _s.empty(); }

  bool startsWith(const char* prefix) const { return _s.find(prefix) == 0; }
  bool startsWith(const String& prefix) const { return _s.find(prefix._s) == 0; }

  int indexOf(const char* s, int from = 0) const
  {
    auto pos = _s.find(s, from);
    return pos == std::string::npos ? -1 : (int)pos;
  }
  int indexOf(const String& s, int from = 0) const
  {
    auto pos = _s.find(s._s, from);
    return pos == std::string::npos ? -1 : (int)pos;
  }
  int indexOf(char c, int from = 0) const
  {
    auto pos = _s.find(c, from);
    return pos == std::string::npos ? -1 : (int)pos;
  }

  const char* c_str() const { return _s.c_str(); }
  size_t length() const { return _s.length(); }

  String operator+(const String& other) const { return String(_s + other._s); }
  String operator+(const char* other) const { return String(_s + other); }
  String& operator+=(const String& other)
  {
    _s += other._s;
    return *this;
  }
  String& operator+=(const char* other)
  {
    _s += other;
    return *this;
  }

  bool operator==(const String& other) const { return _s == other._s; }
  bool operator!=(const String& other) const { return _s != other._s; }
  bool operator==(const char* other) const { return _s == other; }
  bool operator!=(const char* other) const { return _s != other; }

  friend std::ostream& operator<<(std::ostream& os, const String& s) { return os << s._s; }

  private:
  std::string _s;
};

inline String operator+(const char* lhs, const String& rhs)
{
  return String(std::string(lhs) + rhs.c_str());
}
