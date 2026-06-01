/**
 * @file clock_controller.cpp
 * @brief Implementation of the clock controller — NTP/RTC initialization and time display.
 */

#include "clock_controller.h"

/**
 * @brief Initializes the controller with pointers to the view and model.
 * @param _view Pointer to the clock view.
 * @param _model Pointer to the clock model.
 */
Clock_controller::Clock_controller(Clock_view* _view, Clock_model* _model)
: view(_view)
, model(_model)
{
  last_day = -1;
}

/**
 * @brief Initializes the clock by synchronizing time from NTP and setting the DS1307 RTC.
 */
void Clock_controller::setup_clock()
{
  // Wifi_Config wifi_config;
  // model->get_wifi_config(wifi_config);
  configTime(0, 0, config::time_server);
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  tm time_info;
  if (!getLocalTime(&time_info))
  {
    Serial.println("Błąd pobierania czasu!");
  }
  else
  {
    Serial.println("Czas pobrany z NTP:");
    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d\n",
                  time_info.tm_year + 1900,
                  time_info.tm_mon + 1,
                  time_info.tm_mday,
                  time_info.tm_hour,
                  time_info.tm_min,
                  time_info.tm_sec);
  }

  Wire.begin(config::sda_pin, config::scl_pin);

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
  }
  DateTime dt(time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec);
  rtc.adjust(dt);
}

/**
 * @brief Reads the current time from the RTC.
 * @param dt Reference to the DateTime structure that will receive the current time.
 */
void Clock_controller::get_time(DateTime& dt)
{
  dt = rtc.now();
}

/**
 * @brief Updates the clock view — time and date (on day change).
 */
void Clock_controller::update_view()
{
  DateTime now = rtc.now();
  view->show_time(now);
  if (last_day != now.day())
  {
    view->show_date(get_date_string(now, 0), 0);
    for (size_t i = 0; i < 3; ++i)
    {
      view->show_date(get_date_string(now, i), i + 1);
    }
    last_day = now.day();
  }
}

/**
 * @brief Checks whether the given time (hour and minute) matches the current RTC time.
 * @param dt Reference to the DateTime with the time to compare.
 * @return true if the hour and minute match the current time.
 */
bool Clock_controller::is_it_now(DateTime& dt)
{
  DateTime now = rtc.now();
  if (now.hour() == dt.hour() && now.minute() == dt.minute())
  {
    return true;
  }
  return false;
}

/**
 * @brief Returns the date in "DD-MM" format offset by the given number of days.
 * @param dt Base date.
 * @param offset Number of days to add to the base date.
 * @return Pointer to a static buffer containing the date in "DD-MM" format.
 */
const char* Clock_controller::get_date_string(DateTime dt, uint8_t offset)
{
  static char dateStr[11];
  DateTime dt_sum = dt + TimeSpan(offset, 0, 0, 0);
  snprintf(dateStr, sizeof(dateStr), "%02d.%02d", dt_sum.day(), dt_sum.month());
  return dateStr;
}