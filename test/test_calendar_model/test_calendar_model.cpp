#include "test_calendar_model.h"

#include "calendar_model.cpp"

// --- Calendar_model ---

TEST_F(Calendar_model_test, clear_on_empty_model_does_not_crash)
{
  uut.clear();
  EXPECT_EQ(0, uut.get_event_count());
}

TEST_F(Calendar_model_test, update_increases_event_count)
{
  String name("Meeting"), cal("");
  Simple_time ts(10, 0), te(11, 0);
  Calendar_event ev(name, cal, ts, te);

  uut.update(ev);

  EXPECT_EQ(1, uut.get_event_count());
}

TEST_F(Calendar_model_test, update_multiple_events)
{
  String name("Ev"), cal("");
  for (int i = 0; i < 3; i++)
  {
    Simple_time ts((uint8_t)i, 0), te((uint8_t)(i + 1), 0);
    Calendar_event ev(name, cal, ts, te);
    uut.update(ev);
  }

  EXPECT_EQ(3, uut.get_event_count());
}

TEST_F(Calendar_model_test, get_event_returns_correct_data)
{
  String name("Stand-up"), cal("Work");
  Simple_time ts(9, 15), te(9, 30);
  Calendar_event ev(name, cal, ts, te);
  uut.update(ev);

  Calendar_event out;
  uut.get_event(out, 0);

  EXPECT_EQ(String("Stand-up"), out.name);
  EXPECT_EQ(9, out.time_start.hour);
  EXPECT_EQ(15, out.time_start.minutes);
  EXPECT_EQ(9, out.time_stop.hour);
  EXPECT_EQ(30, out.time_stop.minutes);
}

TEST_F(Calendar_model_test, clear_removes_all_events)
{
  String name("Ev"), cal("");
  Simple_time ts(8, 0), te(9, 0);
  Calendar_event ev(name, cal, ts, te);
  uut.update(ev);
  uut.update(ev);

  uut.clear();

  EXPECT_EQ(0, uut.get_event_count());
}

TEST_F(Calendar_model_test, event_added_after_clear_is_accessible)
{
  String name("Old"), cal("");
  Simple_time ts(8, 0), te(9, 0);
  Calendar_event old_ev(name, cal, ts, te);
  uut.update(old_ev);
  uut.clear();

  String name2("New");
  Simple_time ts2(10, 0), te2(11, 0);
  Calendar_event new_ev(name2, cal, ts2, te2);
  uut.update(new_ev);

  EXPECT_EQ(1, uut.get_event_count());
  Calendar_event out;
  uut.get_event(out, 0);
  EXPECT_EQ(String("New"), out.name);
}

TEST_F(Calendar_model_test, set_and_get_config)
{
  google_api_config cfg;
  cfg.ical_url = "https://example.com/cal.ics";
  cfg.ical_alarm_url = "https://example.com/alarm.ics";

  uut.set_config(cfg);

  google_api_config out;
  uut.get_config(out);
  EXPECT_EQ(String("https://example.com/cal.ics"), out.ical_url);
  EXPECT_EQ(String("https://example.com/alarm.ics"), out.ical_alarm_url);
}

// --- Calendar_event ---

TEST_F(Calendar_model_test, get_calendar_label_formats_correctly)
{
  String name("Daily"), cal("");
  Simple_time ts(7, 30), te(8, 0);
  Calendar_event ev(name, cal, ts, te);

  EXPECT_EQ(String("07:30 Daily"), ev.get_calendar_label());
}

TEST_F(Calendar_model_test, get_calendar_label_midnight_event)
{
  String name("Backup"), cal("");
  Simple_time ts(0, 0), te(0, 30);
  Calendar_event ev(name, cal, ts, te);

  EXPECT_EQ(String("00:00 Backup"), ev.get_calendar_label());
}
