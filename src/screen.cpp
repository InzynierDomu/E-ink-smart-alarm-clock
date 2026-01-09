#include "screen.h"

uint8_t Screen::lvBuffer[2][config::lv_buffer] = {0};

Screen::Screen()
: display(GxEPD2_579_GDEY0579T93(45, 46, 47, 48))
{}
void Screen::setup_screen()
{
  pinMode(config::screen_power_pin, OUTPUT);
  digitalWrite(config::screen_power_pin, HIGH);

  epd_setup();

  lv_init();
  lv_tick_set_cb(my_tick);

  lv_display_t* disp = lv_display_create(config::screen_width, config::screen_height);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_user_data(disp, this);
  lv_display_set_buffers(disp, lvBuffer[0], lvBuffer[1], config::lv_buffer, LV_DISPLAY_RENDER_MODE_PARTIAL);

  ui_init();

  lv_obj_set_style_text_color(ui_Screen1, lv_color_make(0x00, 0x00, 0x00), LV_PART_MAIN | LV_STATE_DEFAULT);
}

void Screen::my_disp_flush(lv_display_t* disp, const lv_area_t* area, unsigned char* data)
{
  Screen* screen = static_cast<Screen*>(lv_display_get_user_data(disp));
  if (screen)
  {
    int16_t width = area->x2 - area->x1 + 1;
    int16_t height = area->y2 - area->y1 + 1;

    uint8_t* pixel_data_start = (uint8_t*)data + 8;

    rotate_bitmap_180(pixel_data_start, width, height);

    int16_t display_total_width = screen->display.width();
    int16_t display_total_height = screen->display.height();
    int16_t new_x = display_total_width - (area->x1 + width);
    int16_t new_y = display_total_height - (area->y1 + height);

    screen->display.drawImage(pixel_data_start, new_x, new_y, width, height);
  }
  lv_display_flush_ready(disp);
}

void Screen::rotate_bitmap_180(uint8_t* buffer, int16_t width, int16_t height)
{
  int16_t bytesPerRow = (width + 7) / 8;

  uint8_t* tempBuffer = (uint8_t*)malloc(bytesPerRow * height);
  if (!tempBuffer)
  {
    return;
  }

  for (int16_t y = 0; y < height; y++)
  {
    for (int16_t xByte = 0; xByte < bytesPerRow; xByte++)
    {
      uint8_t originalByte = buffer[y * bytesPerRow + xByte];
      uint8_t reversedByte = 0;
      for (int i = 0; i < 8; i++)
      {
        if ((originalByte >> i) & 1)
        {
          reversedByte |= (1 << (7 - i));
        }
      }
      int16_t rotatedY = height - 1 - y;
      int16_t rotatedXByte = bytesPerRow - 1 - xByte;
      tempBuffer[rotatedY * bytesPerRow + rotatedXByte] = reversedByte;
    }
  }
  memcpy(buffer, tempBuffer, bytesPerRow * height);

  free(tempBuffer);
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
  } while (display.nextPage());
  delay(1000);
}


uint32_t Screen::my_tick(void)
{
  return millis();
}

void Screen::full_clear()
{
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}
