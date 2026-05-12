#include "test_calendar_parser.h"

#include "calendar_model.cpp"
#include "calendar_parser.cpp"

// --- parse_hhmm ---

TEST_F(Calendar_parser_test, parse_valid_time)
{
  Simple_time t = parse_hhmm("10:30");
  EXPECT_EQ(10, t.hour);
  EXPECT_EQ(30, t.minutes);
}

TEST_F(Calendar_parser_test, parse_midnight)
{
  Simple_time t = parse_hhmm("00:00");
  EXPECT_EQ(0, t.hour);
  EXPECT_EQ(0, t.minutes);
}

TEST_F(Calendar_parser_test, parse_end_of_day)
{
  Simple_time t = parse_hhmm("23:59");
  EXPECT_EQ(23, t.hour);
  EXPECT_EQ(59, t.minutes);
}

TEST_F(Calendar_parser_test, parse_single_digit_hour)
{
  Simple_time t = parse_hhmm("9:05");
  EXPECT_EQ(9, t.hour);
  EXPECT_EQ(5, t.minutes);
}

TEST_F(Calendar_parser_test, parse_missing_colon_returns_zero)
{
  Simple_time t = parse_hhmm("1030");
  EXPECT_EQ(0, t.hour);
  EXPECT_EQ(0, t.minutes);
}

TEST_F(Calendar_parser_test, parse_empty_string_returns_zero)
{
  Simple_time t = parse_hhmm("");
  EXPECT_EQ(0, t.hour);
  EXPECT_EQ(0, t.minutes);
}

// --- parse_ical_json ---

TEST_F(Calendar_parser_test, parse_valid_single_event)
{
  String json = "[{\"summary\":\"Meeting\",\"start\":\"10:00\",\"end\":\"11:00\"}]";
  auto events = parse_ical_json(json);
  EXPECT_EQ(1u, events.size());
}

TEST_F(Calendar_parser_test, parse_valid_event_fields)
{
  String json = "[{\"summary\":\"Stand-up\",\"start\":\"09:15\",\"end\":\"09:30\"}]";
  auto events = parse_ical_json(json);

  ASSERT_EQ(1u, events.size());
  EXPECT_EQ(String("Stand-up"), events[0].name);
  EXPECT_EQ(9, events[0].time_start.hour);
  EXPECT_EQ(15, events[0].time_start.minutes);
  EXPECT_EQ(9, events[0].time_stop.hour);
  EXPECT_EQ(30, events[0].time_stop.minutes);
}

TEST_F(Calendar_parser_test, parse_multiple_events_returns_all)
{
  String json = "[{\"summary\":\"A\",\"start\":\"08:00\",\"end\":\"09:00\"},"
                "{\"summary\":\"B\",\"start\":\"10:00\",\"end\":\"11:00\"},"
                "{\"summary\":\"C\",\"start\":\"13:00\",\"end\":\"14:00\"}]";
  auto events = parse_ical_json(json);
  EXPECT_EQ(3u, events.size());
}

TEST_F(Calendar_parser_test, parse_multiple_events_preserves_order)
{
  String json = "[{\"summary\":\"First\",\"start\":\"08:00\",\"end\":\"09:00\"},"
                "{\"summary\":\"Second\",\"start\":\"14:00\",\"end\":\"15:00\"}]";
  auto events = parse_ical_json(json);

  ASSERT_EQ(2u, events.size());
  EXPECT_EQ(String("First"), events[0].name);
  EXPECT_EQ(String("Second"), events[1].name);
}

TEST_F(Calendar_parser_test, parse_empty_array_returns_empty)
{
  auto events = parse_ical_json("[]");
  EXPECT_TRUE(events.empty());
}

TEST_F(Calendar_parser_test, parse_non_json_returns_empty)
{
  auto events = parse_ical_json("not json at all");
  EXPECT_TRUE(events.empty());
}

TEST_F(Calendar_parser_test, parse_malformed_json_returns_empty)
{
  auto events = parse_ical_json("{\"invalid");
  EXPECT_TRUE(events.empty());
}

TEST_F(Calendar_parser_test, parse_object_instead_of_array_returns_empty)
{
  auto events = parse_ical_json("{\"summary\":\"Meeting\",\"start\":\"10:00\",\"end\":\"11:00\"}");
  EXPECT_TRUE(events.empty());
}
