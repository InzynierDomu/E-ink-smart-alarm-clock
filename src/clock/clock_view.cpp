/**
 * @file clock_view.cpp
 * @brief Implementation of the clock view — displaying time and dates via LVGL.
 */

#include "clock_view.h"

/**
 * @brief Initializes the view with a pointer to the screen object.
 * @param scr Pointer to the Screen object.
 */
Clock_view::Clock_view(Screen* scr)
: screen(scr)
{}

/**
 * @brief Displays the current time in "HH:MM" format on an LVGL label.
 * @param now Reference to the current RTC time.
 */
void Clock_view::show_time(DateTime& now)
{
  char buf[5];
  sprintf(buf, "%02d:%02d", now.hour(), now.minute());
  lv_label_set_text(ui_labtime, buf);
}

/**
 * @brief Displays a date on the LVGL label identified by the offset index.
 * @param dateStr String containing the date to display.
 * @param offset Label index (0 = today, 1–3 = following days).
 */
void Clock_view::show_date(const char* dateStr, uint8_t offset)
{
  lv_label_set_text(calendar_labels[offset], dateStr);
}

/**
 * @brief Initializes the list of LVGL label pointers for date display.
 */
void Clock_view::setup_calendar_list()
{
  calendar_labels.push_back(ui_labDate);
  calendar_labels.push_back(ui_labDateDay1);
  calendar_labels.push_back(ui_labDateDay2);
  calendar_labels.push_back(ui_labDateDay3);
}