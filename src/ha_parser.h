/**
 * @file ha_parser.h
 * @brief Free functions for parsing Home Assistant HTTP responses.
 */

#pragma once
#include <Arduino.h>

/**
 * @brief Checks whether the HTTP response headers indicate a 200 OK status.
 * @param headers Full response headers string read from the TCP connection.
 * @return true if the first line starts with "HTTP/1.1 200".
 */
bool is_ha_response_ok(const String& headers);

/**
 * @brief Parses the temperature value from a Home Assistant state API response body.
 * @param body JSON response body from the HA /api/states/<entity> endpoint.
 * @return Temperature as int8_t rounded to the nearest degree, or 0 if parsing fails.
 */
int8_t parse_ha_state(const String& body);
