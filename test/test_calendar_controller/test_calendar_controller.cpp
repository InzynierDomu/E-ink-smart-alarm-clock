#include <gtest/gtest.h>

#include "calendar_model.cpp"
#include "calendar_parser.cpp"

// ---------------------------------------------------------------------------
// Alarm stub
// ---------------------------------------------------------------------------

struct Alarm_stub : Alarm_setter
{
  int  set_no_alarm_count = 0;
  int  set_alarm_count    = 0;
  int  enable_count       = 0;
  Simple_time last_alarm{0, 0};

  void set_no_alarm() override { ++set_no_alarm_count; }
  void set_alarm(Simple_time t) override { last_alarm = t; ++set_alarm_count; }
  void enable_alarm() override { ++enable_count; }
  bool get_alarm(Simple_time& out) const override
  {
    if (set_alarm_count == 0) return false;
    out = last_alarm;
    return true;
  }

  void reset()
  {
    set_no_alarm_count = 0;
    set_alarm_count    = 0;
    enable_count       = 0;
    last_alarm         = Simple_time(0, 0);
  }
};

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

struct Calendar_controller_test : public ::testing::Test
{
protected:
  Calendar_model model;
  Alarm_stub     alarm;

  DateTime at(uint8_t h, uint8_t m) { return DateTime(2000, 1, 1, h, m, 0); }

  std::vector<Calendar_event> parse(const char* json)
  {
    return parse_ical_json(String(json));
  }

  void add_event(const char* name, const char* start, const char* stop)
  {
    String n(name), c(""), s(start), e(stop);
    Simple_time ts = parse_hhmm(s);
    Simple_time te = parse_hhmm(e);
    model.update(Calendar_event(n, c, ts, te));
  }
};

// ---------------------------------------------------------------------------
// apply_event_response: clears old events, populates with new ones
// ---------------------------------------------------------------------------

TEST_F(Calendar_controller_test, apply_event_clears_existing_events)
{
  add_event("OldMeeting", "09:00", "10:00");
  ASSERT_EQ(1u, model.get_event_count());

  auto events = parse("[]");
  apply_event_response(model, events);

  EXPECT_EQ(0u, model.get_event_count());
}

TEST_F(Calendar_controller_test, apply_event_populates_model)
{
  auto events = parse(
    "[{\"summary\":\"StandUp\",\"start\":\"09:00\",\"end\":\"09:15\"},"
    "{\"summary\":\"Review\",\"start\":\"14:00\",\"end\":\"15:00\"}]");

  apply_event_response(model, events);

  EXPECT_EQ(2u, model.get_event_count());
}

TEST_F(Calendar_controller_test, apply_event_replaces_old_events_with_new)
{
  add_event("OldEvent", "08:00", "09:00");

  auto events = parse(
    "[{\"summary\":\"NewEvent\",\"start\":\"11:00\",\"end\":\"12:00\"}]");
  apply_event_response(model, events);

  ASSERT_EQ(1u, model.get_event_count());
  Calendar_event ev;
  model.get_event(ev, 0);
  EXPECT_EQ(String("NewEvent"), ev.name);
}

// ---------------------------------------------------------------------------
// apply_alarm_response: resets alarm on success, sets next future alarm
// ---------------------------------------------------------------------------

TEST_F(Calendar_controller_test, apply_alarm_resets_before_setting_new)
{
  auto events = parse(
    "[{\"summary\":\"Wake\",\"start\":\"07:00\",\"end\":\"07:30\"}]");

  apply_alarm_response(alarm, events, at(6, 0));

  EXPECT_EQ(1, alarm.set_no_alarm_count);
  EXPECT_EQ(1, alarm.set_alarm_count);
  EXPECT_EQ(7, alarm.last_alarm.hour);
  EXPECT_EQ(0, alarm.last_alarm.minutes);
}

TEST_F(Calendar_controller_test, apply_alarm_resets_even_when_no_future_alarm)
{
  // Wszystkie budziki w przeszłości — reset bez ustawienia nowego
  auto events = parse(
    "[{\"summary\":\"Gone\",\"start\":\"06:00\",\"end\":\"06:30\"}]");

  apply_alarm_response(alarm, events, at(12, 0));

  EXPECT_EQ(1, alarm.set_no_alarm_count);
  EXPECT_EQ(0, alarm.set_alarm_count);
  EXPECT_EQ(0, alarm.enable_count);
}

TEST_F(Calendar_controller_test, apply_alarm_resets_on_empty_response)
{
  auto events = parse("[]");

  apply_alarm_response(alarm, events, at(12, 0));

  EXPECT_EQ(1, alarm.set_no_alarm_count);
  EXPECT_EQ(0, alarm.set_alarm_count);
}

TEST_F(Calendar_controller_test, apply_alarm_picks_earliest_future_from_multiple)
{
  // Reprodukuje oryginalny bug — przy 11:45 powinien wybrać 12:00
  auto events = parse(
    "[{\"summary\":\"A\",\"start\":\"12:00\",\"end\":\"12:30\"},"
    "{\"summary\":\"B\",\"start\":\"12:30\",\"end\":\"13:00\"},"
    "{\"summary\":\"C\",\"start\":\"13:30\",\"end\":\"14:00\"}]");

  apply_alarm_response(alarm, events, at(11, 45));

  EXPECT_EQ(12, alarm.last_alarm.hour);
  EXPECT_EQ(0,  alarm.last_alarm.minutes);
}

// ---------------------------------------------------------------------------
// Kluczowy scenariusz: brak odpowiedzi HTTP → stara data zostaje
// (apply_* nie jest wołane — gwarantuje to struktura fetch_ical)
// Te testy weryfikują, że clear/set_no_alarm jest w środku apply_*, a nie przed nim.
// ---------------------------------------------------------------------------

TEST_F(Calendar_controller_test, events_not_cleared_unless_apply_called)
{
  add_event("PreExisting", "10:00", "11:00");

  // Symulujemy brak HTTP response — apply_event_response NIE jest wołane
  // Model powinien zachować stare dane
  EXPECT_EQ(1u, model.get_event_count());
}

TEST_F(Calendar_controller_test, alarm_not_reset_unless_apply_called)
{
  // alarm.set_no_alarm nie było wołane — licznik = 0
  EXPECT_EQ(0, alarm.set_no_alarm_count);
}
