/**
 * @file calendar_parser.h
 * @brief Free functions for parsing iCal-proxy JSON responses into calendar data structures.
 */

#pragma once
#include "calendar_model.h"
#include <vector>

/**
 * @brief Parses a time string in "HH:MM" format into a Simple_time structure.
 * @param hhmm String containing the time in "HH:MM" format.
 * @return Parsed time as Simple_time; (0,0) on parse error.
 */
Simple_time parse_hhmm(const String& hhmm);

/**
 * @brief Parses a JSON array of calendar events returned by the iCal proxy.
 * @param json JSON string — expected to be an array of objects with "summary", "start", "end" fields.
 * @return Vector of parsed Calendar_event; empty on any error or if input is empty.
 */
std::vector<Calendar_event> parse_ical_json(const String& json);
