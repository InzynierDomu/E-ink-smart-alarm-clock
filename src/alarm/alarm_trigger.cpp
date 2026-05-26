/**
 * @file alarm_trigger.cpp
 * @brief Implementation of the alarm trigger sequence.
 */

#include "alarm_trigger.h"

#include "logger.h"


/**
 * @brief Initializes the trigger with all injectable dependencies.
 * @param chk   Alarm time check interface.
 * @param mq    MQTT notification interface.
 * @param aud   Audio control interface.
 * @param flag  FreeRTOS flag that signals the audio task to play.
 */
Alarm_trigger::Alarm_trigger(Alarm_check& chk, Alarm_mqtt& mq, Alarm_audio& aud, volatile bool& flag)
: check_iface(chk)
, mqtt(mq)
, audio_ctrl(aud)
, start_flag(flag)
{}

bool Alarm_trigger::try_trigger(const DateTime& now, bool is_ap_mode)
{
  if (active || !check_iface.check(now))
    return false;

  if (!is_ap_mode)
    mqtt.send_action();

  Logger::info("ALARM", "Alarm triggered");
  audio_ctrl.start();
  start_flag = true;
  active = true;
  return true;
}

void Alarm_trigger::stop()
{
  if (!active)
    return;

  audio_ctrl.stop();
  start_flag = false;
  active = false;
  Logger::info("ALARM", "Alarm stopped");
}

bool Alarm_trigger::is_active() const
{
  return active;
}
