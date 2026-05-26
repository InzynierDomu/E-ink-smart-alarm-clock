/**
 * @file SD.h
 * @brief Stub replacing the Arduino SD library for native unit tests.
 *        Tracks how many times open() is called so tests can assert
 *        whether the logger attempted SD card access.
 */

#pragma once
#include "Arduino.h"

#define FILE_APPEND "a"

struct File
{
  bool _valid = false;
  explicit File(bool valid = false) : _valid(valid) {}
  operator bool() const { return _valid; }
  size_t size() { return 0; }
  void close() {}
  size_t print(const String&) { return 0; }
  size_t print(const char*) { return 0; }
};

struct SD_Class
{
  int open_count = 0;
  bool exists_result = false;

  File open(const String&, const char*) { open_count++; return File{false}; }
  bool exists(const String&) { return exists_result; }
  void remove(const String&) {}

  void reset() { open_count = 0; exists_result = false; }
};

inline SD_Class SD;
