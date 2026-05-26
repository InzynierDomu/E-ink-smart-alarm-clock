#include <gtest/gtest.h>

#include "calendar_model.cpp"
#include "calendar_parser.cpp"

struct Calendar_alarm_test : public ::testing::Test
{
protected:
  DateTime at(uint8_t hour, uint8_t minute)
  {
    return DateTime(2000, 1, 1, hour, minute, 0);
  }

  std::vector<Calendar_event> parse(const char* json)
  {
    return parse_ical_json(String(json));
  }
};

// ---------------------------------------------------------------------------
// Główny scenariusz bugowy: 4 budziki, wybieramy najbliższy przyszły
// ---------------------------------------------------------------------------

TEST_F(Calendar_alarm_test, picks_earliest_future_alarm_from_four)
{
  // Był bug: zawsze wybierał ostatni (13:30). Przy 11:45 powinien wybrać 12:00.
  auto events = parse(
    "[{\"summary\":\"A\",\"start\":\"12:00\",\"end\":\"12:30\"},"
    "{\"summary\":\"B\",\"start\":\"12:30\",\"end\":\"13:00\"},"
    "{\"summary\":\"C\",\"start\":\"13:00\",\"end\":\"13:30\"},"
    "{\"summary\":\"D\",\"start\":\"13:30\",\"end\":\"14:00\"}]");

  Simple_time result(0, 0);
  ASSERT_TRUE(select_next_alarm(events, at(11, 45), result));
  EXPECT_EQ(12, result.hour);
  EXPECT_EQ(0,  result.minutes);
}

TEST_F(Calendar_alarm_test, skips_past_alarms_picks_next)
{
  // 12:00 już minęło — powinien wybrać 12:30
  auto events = parse(
    "[{\"summary\":\"A\",\"start\":\"12:00\",\"end\":\"12:30\"},"
    "{\"summary\":\"B\",\"start\":\"12:30\",\"end\":\"13:00\"},"
    "{\"summary\":\"C\",\"start\":\"13:00\",\"end\":\"13:30\"},"
    "{\"summary\":\"D\",\"start\":\"13:30\",\"end\":\"14:00\"}]");

  Simple_time result(0, 0);
  ASSERT_TRUE(select_next_alarm(events, at(12, 15), result));
  EXPECT_EQ(12, result.hour);
  EXPECT_EQ(30, result.minutes);
}

TEST_F(Calendar_alarm_test, skips_two_past_picks_third)
{
  auto events = parse(
    "[{\"summary\":\"A\",\"start\":\"12:00\",\"end\":\"12:30\"},"
    "{\"summary\":\"B\",\"start\":\"12:30\",\"end\":\"13:00\"},"
    "{\"summary\":\"C\",\"start\":\"13:00\",\"end\":\"13:30\"},"
    "{\"summary\":\"D\",\"start\":\"13:30\",\"end\":\"14:00\"}]");

  Simple_time result(0, 0);
  ASSERT_TRUE(select_next_alarm(events, at(12, 45), result));
  EXPECT_EQ(13, result.hour);
  EXPECT_EQ(0,  result.minutes);
}

// ---------------------------------------------------------------------------
// Brak przyszłych budzików
// ---------------------------------------------------------------------------

TEST_F(Calendar_alarm_test, returns_false_when_all_alarms_past)
{
  auto events = parse(
    "[{\"summary\":\"A\",\"start\":\"08:00\",\"end\":\"08:30\"},"
    "{\"summary\":\"B\",\"start\":\"09:00\",\"end\":\"09:30\"}]");

  Simple_time result(0, 0);
  EXPECT_FALSE(select_next_alarm(events, at(10, 0), result));
}

TEST_F(Calendar_alarm_test, returns_false_for_empty_event_list)
{
  std::vector<Calendar_event> events;
  Simple_time result(0, 0);
  EXPECT_FALSE(select_next_alarm(events, at(12, 0), result));
}

TEST_F(Calendar_alarm_test, returns_false_for_empty_json_array)
{
  auto events = parse("[]");
  Simple_time result(0, 0);
  EXPECT_FALSE(select_next_alarm(events, at(12, 0), result));
}

// ---------------------------------------------------------------------------
// Jeden budzik
// ---------------------------------------------------------------------------

TEST_F(Calendar_alarm_test, single_future_alarm_is_selected)
{
  auto events = parse(
    "[{\"summary\":\"Wake\",\"start\":\"07:00\",\"end\":\"07:30\"}]");

  Simple_time result(0, 0);
  ASSERT_TRUE(select_next_alarm(events, at(6, 0), result));
  EXPECT_EQ(7, result.hour);
  EXPECT_EQ(0, result.minutes);
}

TEST_F(Calendar_alarm_test, single_past_alarm_returns_false)
{
  auto events = parse(
    "[{\"summary\":\"Wake\",\"start\":\"07:00\",\"end\":\"07:30\"}]");

  Simple_time result(0, 0);
  EXPECT_FALSE(select_next_alarm(events, at(7, 30), result));
}

// ---------------------------------------------------------------------------
// Granica: budzik dokładnie o aktualnej godzinie nie jest "przyszły"
// ---------------------------------------------------------------------------

TEST_F(Calendar_alarm_test, alarm_at_exact_current_time_not_selected)
{
  // Logika używa > (ściśle większy) — budzik na dokładnie teraz jest pomijany
  auto events = parse(
    "[{\"summary\":\"Wake\",\"start\":\"08:00\",\"end\":\"08:30\"}]");

  Simple_time result(0, 0);
  EXPECT_FALSE(select_next_alarm(events, at(8, 0), result));
}

// ---------------------------------------------------------------------------
// Kolejność w JSON nie ma znaczenia — zawsze wybieramy najwcześniejszy
// ---------------------------------------------------------------------------

TEST_F(Calendar_alarm_test, picks_earliest_regardless_of_json_order)
{
  // Budziki podane od najpóźniejszego do najwcześniejszego
  auto events = parse(
    "[{\"summary\":\"D\",\"start\":\"13:30\",\"end\":\"14:00\"},"
    "{\"summary\":\"C\",\"start\":\"13:00\",\"end\":\"13:30\"},"
    "{\"summary\":\"B\",\"start\":\"12:30\",\"end\":\"13:00\"},"
    "{\"summary\":\"A\",\"start\":\"12:00\",\"end\":\"12:30\"}]");

  Simple_time result(0, 0);
  ASSERT_TRUE(select_next_alarm(events, at(11, 45), result));
  EXPECT_EQ(12, result.hour);
  EXPECT_EQ(0,  result.minutes);
}
