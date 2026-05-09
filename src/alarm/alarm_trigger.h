/**
 * @file alarm_trigger.h
 * @brief Alarm trigger — orchestrates MQTT, logger mute, and audio on alarm activation.
 */

#pragma once
#include "RTClib.h"

/**
 * @brief Interface for checking whether the alarm should fire at the given time.
 */
struct Alarm_check
{
  virtual bool check(DateTime& now) = 0;
  virtual ~Alarm_check() = default;
};

/**
 * @brief Interface for sending an MQTT notification on alarm trigger.
 */
struct Alarm_mqtt
{
  virtual void send_action() = 0;
  virtual ~Alarm_mqtt() = default;
};

/**
 * @brief Interface for starting and stopping alarm audio playback.
 */
struct Alarm_audio
{
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual ~Alarm_audio() = default;
};

/**
 * @brief Orchestrates the alarm trigger sequence: check → optional MQTT → logger mute → audio start.
 */
class Alarm_trigger
{
public:
  Alarm_trigger(Alarm_check& check, Alarm_mqtt& mqtt, Alarm_audio& audio, volatile bool& start_flag);

  /**
   * @brief Tries to activate the alarm. No-op if already active or alarm time not reached.
   * @param now Current RTC time.
   * @param is_ap_mode When true, MQTT notification is skipped.
   * @return true if the alarm just activated.
   */
  bool try_trigger(DateTime& now, bool is_ap_mode);

  /**
   * @brief Stops the alarm: halts audio, unmutes logger, clears the active flag.
   */
  void stop();

  /**
   * @brief Returns true while the alarm is active (between trigger and stop).
   */
  bool is_active() const;

private:
  Alarm_check& check_iface;  ///< Alarm time check dependency.
  Alarm_mqtt& mqtt;          ///< MQTT notification dependency.
  Alarm_audio& audio_ctrl;   ///< Audio control dependency.
  volatile bool& start_flag; ///< FreeRTOS task flag driving audio playback.
  bool active = false;       ///< True while alarm is sounding.
};
