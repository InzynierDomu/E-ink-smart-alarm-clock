#include <gtest/gtest.h>

#include "alarm_model.h"
#include "alarm_model.cpp"

// check_alarm() logic lives in Alarm_model, so we test it here directly —
// no Alarm_view or screen.h dependency needed.

struct Alarm_check_test : public ::testing::Test
{
protected:
  Alarm_model uut;

  void set_enabled(uint8_t hour, uint8_t minute)
  {
    Clock_alarm alarm;
    alarm.time   = Simple_time(hour, minute);
    alarm.enable = true;
    uut.add_alarm(alarm);
  }

  DateTime at(uint8_t hour, uint8_t minute)
  {
    return DateTime(2000, 1, 1, hour, minute, 0);
  }
};

// ---------------------------------------------------------------------------
// Default state
// ---------------------------------------------------------------------------

TEST_F(Alarm_check_test, no_alarm_by_default)
{
  EXPECT_FALSE(uut.check_alarm(at(0, 0)));
}

TEST_F(Alarm_check_test, disabled_alarm_does_not_trigger_at_alarm_time)
{
  // set_alarm() alone leaves enable=false — must call enable_alarm() separately
  Clock_alarm alarm;
  alarm.time   = Simple_time(7, 0);
  alarm.enable = false;
  uut.add_alarm(alarm);

  EXPECT_FALSE(uut.check_alarm(at(7, 0)));
}

// ---------------------------------------------------------------------------
// Normal trigger
// ---------------------------------------------------------------------------

TEST_F(Alarm_check_test, triggers_when_time_matches)
{
  set_enabled(7, 0);
  EXPECT_TRUE(uut.check_alarm(at(7, 0)));
}

TEST_F(Alarm_check_test, does_not_trigger_one_minute_early)
{
  set_enabled(7, 0);
  EXPECT_FALSE(uut.check_alarm(at(6, 59)));
}

TEST_F(Alarm_check_test, does_not_trigger_one_minute_late)
{
  set_enabled(7, 0);
  EXPECT_FALSE(uut.check_alarm(at(7, 1)));
}

TEST_F(Alarm_check_test, does_not_trigger_wrong_hour_same_minute)
{
  set_enabled(7, 30);
  EXPECT_FALSE(uut.check_alarm(at(8, 30)));
}

// ---------------------------------------------------------------------------
// Kluczowy scenariusz: budzik ustawiony na 7:00, usunięty przed 7
// → brak wyzwolenia o 0:00 (domyślny czas po usunięciu)
// ---------------------------------------------------------------------------

TEST_F(Alarm_check_test, no_trigger_at_midnight_after_alarm_removed)
{
  set_enabled(7, 0);
  uut.set_no_alarm();

  EXPECT_FALSE(uut.check_alarm(at(0, 0)));
}

TEST_F(Alarm_check_test, no_trigger_at_alarm_time_after_removal)
{
  set_enabled(7, 0);
  uut.set_no_alarm();

  EXPECT_FALSE(uut.check_alarm(at(7, 0)));
}

// ---------------------------------------------------------------------------
// Toggle (wyłączenie bez usuwania)
// ---------------------------------------------------------------------------

TEST_F(Alarm_check_test, no_trigger_when_toggled_off)
{
  set_enabled(7, 0);
  uut.toggle_alarm();

  EXPECT_FALSE(uut.check_alarm(at(7, 0)));
}

TEST_F(Alarm_check_test, triggers_again_after_toggle_back_on)
{
  set_enabled(7, 0);
  uut.toggle_alarm();   // wyłącz
  uut.toggle_alarm();   // włącz z powrotem

  EXPECT_TRUE(uut.check_alarm(at(7, 0)));
}

// ---------------------------------------------------------------------------
// Edge case: alarm ustawiony dokładnie na 0:00
// ---------------------------------------------------------------------------

TEST_F(Alarm_check_test, midnight_alarm_triggers_at_midnight)
{
  set_enabled(0, 0);
  EXPECT_TRUE(uut.check_alarm(at(0, 0)));
}

TEST_F(Alarm_check_test, midnight_alarm_does_not_trigger_at_noon)
{
  set_enabled(0, 0);
  EXPECT_FALSE(uut.check_alarm(at(12, 0)));
}
