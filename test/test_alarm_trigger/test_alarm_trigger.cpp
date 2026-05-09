#include <gtest/gtest.h>
#include <memory>

#include "alarm_trigger.h"
#include "alarm_trigger.cpp"

// ---------------------------------------------------------------------------
// Stubs
// ---------------------------------------------------------------------------

struct Alarm_check_stub : Alarm_check
{
  bool result = false;
  bool check(DateTime&) override { return result; }
};

struct Mqtt_stub : Alarm_mqtt
{
  int call_count = 0;
  void send_action() override { call_count++; }
};

struct Audio_stub : Alarm_audio
{
  int start_count = 0;
  int stop_count  = 0;
  void start() override { start_count++; }
  void stop()  override { stop_count++;  }
};

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------

struct Alarm_trigger_test : public ::testing::Test
{
  Alarm_check_stub alarm_check;
  Mqtt_stub        mqtt;
  Audio_stub       audio;
  volatile bool    start_flag = false;
  DateTime         dummy_now;

  std::unique_ptr<Alarm_trigger> trigger;

  void SetUp() override
  {
    Logger::unmute();
    start_flag           = false;
    alarm_check.result   = false;
    mqtt.call_count      = 0;
    audio.start_count    = 0;
    audio.stop_count     = 0;
    trigger.reset(new Alarm_trigger(alarm_check, mqtt, audio, start_flag));
  }
};

// ---------------------------------------------------------------------------
// try_trigger — alarm not set
// ---------------------------------------------------------------------------

TEST_F(Alarm_trigger_test, no_trigger_when_alarm_not_set)
{
  alarm_check.result = false;
  EXPECT_FALSE(trigger->try_trigger(dummy_now, false));
}

TEST_F(Alarm_trigger_test, not_active_when_alarm_not_set)
{
  trigger->try_trigger(dummy_now, false);
  EXPECT_FALSE(trigger->is_active());
}

// ---------------------------------------------------------------------------
// try_trigger — normal mode (WiFi, MQTT available)
// ---------------------------------------------------------------------------

TEST_F(Alarm_trigger_test, returns_true_when_alarm_matches)
{
  alarm_check.result = true;
  EXPECT_TRUE(trigger->try_trigger(dummy_now, false));
}

TEST_F(Alarm_trigger_test, sends_mqtt_in_normal_mode)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  EXPECT_EQ(mqtt.call_count, 1);
}

TEST_F(Alarm_trigger_test, starts_audio_in_normal_mode)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  EXPECT_EQ(audio.start_count, 1);
}

TEST_F(Alarm_trigger_test, sets_start_flag_in_normal_mode)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  EXPECT_TRUE(start_flag);
}

TEST_F(Alarm_trigger_test, mutes_logger_in_normal_mode)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  EXPECT_TRUE(Logger::is_muted());
}

TEST_F(Alarm_trigger_test, is_active_after_trigger)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  EXPECT_TRUE(trigger->is_active());
}

// ---------------------------------------------------------------------------
// try_trigger — AP mode (no MQTT)
// ---------------------------------------------------------------------------

TEST_F(Alarm_trigger_test, ap_mode_still_triggers)
{
  alarm_check.result = true;
  EXPECT_TRUE(trigger->try_trigger(dummy_now, true));
}

TEST_F(Alarm_trigger_test, ap_mode_skips_mqtt)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, true);
  EXPECT_EQ(mqtt.call_count, 0);
}

TEST_F(Alarm_trigger_test, ap_mode_starts_audio)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, true);
  EXPECT_EQ(audio.start_count, 1);
}

TEST_F(Alarm_trigger_test, ap_mode_mutes_logger)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, true);
  EXPECT_TRUE(Logger::is_muted());
}

// ---------------------------------------------------------------------------
// try_trigger — idempotency (already active)
// ---------------------------------------------------------------------------

TEST_F(Alarm_trigger_test, second_try_trigger_returns_false_when_active)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  EXPECT_FALSE(trigger->try_trigger(dummy_now, false));
}

TEST_F(Alarm_trigger_test, audio_started_only_once_on_double_trigger)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  trigger->try_trigger(dummy_now, false);
  EXPECT_EQ(audio.start_count, 1);
}

TEST_F(Alarm_trigger_test, mqtt_sent_only_once_on_double_trigger)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  trigger->try_trigger(dummy_now, false);
  EXPECT_EQ(mqtt.call_count, 1);
}

// ---------------------------------------------------------------------------
// stop
// ---------------------------------------------------------------------------

TEST_F(Alarm_trigger_test, stop_stops_audio)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  trigger->stop();
  EXPECT_EQ(audio.stop_count, 1);
}

TEST_F(Alarm_trigger_test, stop_clears_start_flag)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  trigger->stop();
  EXPECT_FALSE(start_flag);
}

TEST_F(Alarm_trigger_test, stop_unmutes_logger)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  trigger->stop();
  EXPECT_FALSE(Logger::is_muted());
}

TEST_F(Alarm_trigger_test, stop_clears_active_flag)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  trigger->stop();
  EXPECT_FALSE(trigger->is_active());
}

// ---------------------------------------------------------------------------
// full cycle: trigger → stop → retrigger
// ---------------------------------------------------------------------------

TEST_F(Alarm_trigger_test, can_retrigger_after_stop)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  trigger->stop();
  EXPECT_TRUE(trigger->try_trigger(dummy_now, false));
  EXPECT_EQ(audio.start_count, 2);
}

TEST_F(Alarm_trigger_test, logger_muted_again_after_retrigger)
{
  alarm_check.result = true;
  trigger->try_trigger(dummy_now, false);
  trigger->stop();
  trigger->try_trigger(dummy_now, false);
  EXPECT_TRUE(Logger::is_muted());
}
