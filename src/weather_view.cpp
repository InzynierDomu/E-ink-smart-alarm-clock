#include "weather_view.h"
// Dodać #include <lvgl.h> jeśli używasz LVGL

Weather_view::Weather_view(Screen* scr)
: screen(scr)
{
  // Tworzenie LVGL obiektów na screen
  // tempLabel = lv_label_create(screen->getLvObj());
}

void Weather_view::show(const Simple_weather& data)
{
  // Aktualizuj LVGL obiekty danymi
  // String tempStr = String(data.temperature, 1) + "°C";
  // String humStr = String(data.humidity, 1) + "%";
  // lv_label_set_text(tempLabel, tempStr.c_str());
  // lv_label_set_text(descLabel, data.description.c_str());
}