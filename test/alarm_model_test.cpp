#include "alarm_model_test.h"

#include "alarm_model.cpp"

TEST_F(Alarm_model_test, default_has_no_alarm)
{
  EXPECT_FALSE(uut.get_is_alarm());
}

TEST_F(Alarm_model_test, set_alarm_marks_is_alarm)
{
  Clock_alarm alarm;
  alarm.time = Simple_time(7, 30);
  alarm.enable = true;

  uut.set_alarm(alarm, true);

  EXPECT_TRUE(uut.get_is_alarm());
}

TEST_F(Alarm_model_test, set_alarm_stores_time_correctly)
{
  Clock_alarm alarm;
  alarm.time = Simple_time(6, 45);
  alarm.enable = true;

  uut.set_alarm(alarm, true);

  Clock_alarm out;
  uut.get_alarm(out);
  EXPECT_EQ(6, out.time.hour);
  EXPECT_EQ(45, out.time.minutes);
}

TEST_F(Alarm_model_test, set_no_alarm_clears_is_alarm)
{
  Clock_alarm alarm;
  alarm.time = Simple_time(7, 0);
  alarm.enable = true;
  uut.set_alarm(alarm, true);

  uut.set_no_alarm();

  EXPECT_FALSE(uut.get_is_alarm());
}

TEST_F(Alarm_model_test, set_no_alarm_resets_time_to_zero)
{
  Clock_alarm alarm;
  alarm.time = Simple_time(7, 30);
  alarm.enable = true;
  uut.set_alarm(alarm, true);

  uut.set_no_alarm();

  Clock_alarm out;
  uut.get_alarm(out);
  EXPECT_EQ(0, out.time.hour);
  EXPECT_EQ(0, out.time.minutes);
  EXPECT_FALSE(out.enable);
}

TEST_F(Alarm_model_test, toggle_alarm_flips_enable_flag)
{
  Clock_alarm alarm;
  alarm.time = Simple_time(8, 0);
  alarm.enable = false;
  uut.set_alarm(alarm, true);

  uut.toggle_alarm();

  Clock_alarm out;
  uut.get_alarm(out);
  EXPECT_TRUE(out.enable);
}

TEST_F(Alarm_model_test, toggle_alarm_twice_restores_state)
{
  Clock_alarm alarm;
  alarm.time = Simple_time(8, 0);
  alarm.enable = true;
  uut.set_alarm(alarm, true);

  uut.toggle_alarm();
  uut.toggle_alarm();

  Clock_alarm out;
  uut.get_alarm(out);
  EXPECT_TRUE(out.enable);
}

TEST_F(Alarm_model_test, enable_alarm_sets_enable_flag)
{
  Clock_alarm alarm;
  alarm.time = Simple_time(9, 15);
  alarm.enable = false;
  uut.set_alarm(alarm, true);

  uut.enable_alarm();

  Clock_alarm out;
  uut.get_alarm(out);
  EXPECT_TRUE(out.enable);
}

TEST_F(Alarm_model_test, set_alarm_with_false_means_no_alarm)
{
  Clock_alarm alarm;
  alarm.time = Simple_time(7, 0);
  alarm.enable = true;

  uut.set_alarm(alarm, false);

  EXPECT_FALSE(uut.get_is_alarm());
}
