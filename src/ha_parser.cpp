/**
 * @file ha_parser.cpp
 * @brief Implementation of Home Assistant HTTP response parsing utilities.
 */

#include "ha_parser.h"

#include "logger.h"


bool is_ha_response_ok(const String& headers)
{
  return headers.startsWith("HTTP/1.1 200");
}

int8_t parse_ha_state(const String& body)
{
  int idx = body.indexOf("\"state\":");
  if (idx < 0)
  {
    Logger::error("HA", "'state' not found in response body");
    return 0;
  }

  int start = body.indexOf("\"", idx + 8);
  int end = body.indexOf("\"", start + 1);
  if (start < 0 || end < 0 || end <= start)
  {
    Logger::error("HA", "Failed to parse state value from body");
    return 0;
  }

  return body.substring(start + 1, end).toInt();
}
