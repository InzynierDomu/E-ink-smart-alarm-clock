/**
 * @file calendar_parser.h
 * @brief Free functions for parsing iCal-proxy JSON responses into calendar data structures.
 */

#pragma once
#include "calendar_model.h"
#include <RTClib.h>
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

/**
 * @brief Selects the earliest upcoming alarm from a list of events.
 * @param events Parsed calendar events representing alarm times.
 * @param now Current time; only alarms strictly after this moment are considered.
 * @param out Set to the selected alarm time when returning true.
 * @return true if a future alarm was found, false if all events are in the past.
 */
bool select_next_alarm(const std::vector<Calendar_event>& events, const DateTime& now, Simple_time& out);
