#include "screen.h"

void Screen::setup_screen()
{
  pinMode(config::screen_power_pin, OUTPUT);
  digitalWrite(config::screen_power_pin, HIGH);

  epd_setup();

  lv_init();
  lv_tick_set_cb(my_tick);

  lv_display_t* disp = lv_display_create(config::screen_width, config::screen_width);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, lvBuffer[0], lvBuffer[1], config::lv_buffer, LV_DISPLAY_RENDER_MODE_PARTIAL);

  ui_init();

  lv_obj_set_style_text_color(ui_Screen1, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN | LV_STATE_DEFAULT);
}
void Screen::my_disp_flush(lv_display_t* disp, const lv_area_t* area, unsigned char* data)
{
  int16_t width = area->x2 - area->x1 + 1;
  int16_t height = area->y2 - area->y1 + 1;
  display.drawImage((uint8_t*)data + 8, area->x1, area->y1, width, height);

  lv_display_flush_ready(disp);
}

void Screen::epd_setup()
{
  SPI.begin(12, -1, 11, 45);
  display.init(115200, true, 2, false);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setRotation(2);
  } while (display.nextPage());
  delay(1000);
}


static uint32_t Screen::my_tick(void)
{
  return millis();
}