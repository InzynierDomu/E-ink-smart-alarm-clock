#include "RTClib.h"
#include "lvgl.h"
#include "ui/ui.h"
#include <Arduino.h>
#include <GxEPD2_BW.h>

#define SCR_WIDTH 792
#define SCR_HEIGHT 272
#define LVBUF ((SCR_WIDTH * SCR_HEIGHT / 8) + 8)

RTC_DS1307 m_rtc; ///< DS1307 RTC

GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT>
    display(GxEPD2_579_GDEY0579T93(45, 46, 47, 48));

static uint8_t lvBuffer[2][LVBUF];

void my_disp_flush(lv_display_t *disp, const lv_area_t *area,
                   uint8_t *color_p) {
  Serial.print("FLUSH x1=");
  Serial.print(area->x1);
  Serial.print(" y1=");
  Serial.print(area->y1);
  Serial.print(" x2=");
  Serial.print(area->x2);
  Serial.print(" y2=");
  Serial.println(area->y2);

  // konwersja bufora
  int16_t width = area->x2 - area->x1 + 1;
  int16_t height = area->y2 - area->y1 + 1;
  int buf_size = width * height;
  static uint8_t converted_buf[LVBUF] = {0};
  memset(converted_buf, 0x00, sizeof(converted_buf));
  int converted_index = 0;
  uint8_t bit_mask = 0x80;
  for (int i = 0; i < buf_size; i++) {
    uint8_t pixel = color_p[i];
    bool is_black = pixel > 128;
    if (is_black) {
      converted_buf[converted_index] |= bit_mask;
    }
    bit_mask >>= 1;
    if (bit_mask == 0) {
      bit_mask = 0x80;
      converted_index++;
      if (converted_index < sizeof(converted_buf))
        converted_buf[converted_index] = 0;
    }
  }
  display.drawImage(converted_buf, area->x1, area->y1, width, height);
  lv_display_flush_ready(disp);
}

static uint32_t my_tick(void) { return millis(); }

void epd_setup() {
  SPI.begin(12, -1, 11, 45);
  display.init(115200, true, 2, false);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
  delay(1000);
}

static void update_clock(lv_timer_t *timer) {
  DateTime now = m_rtc.now();
  char buf[4];
  sprintf(buf, "%02d:%02d", now.hour(), now.minute());
  lv_label_set_text(ui_labtime, buf);
}

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 38);

  if (!m_rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  m_rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH);

  epd_setup();

  lv_init();
  lv_tick_set_cb(my_tick);

  lv_display_t *disp = lv_display_create(SCR_WIDTH, SCR_HEIGHT);
  lv_display_set_flush_cb(disp, my_disp_flush);
  lv_display_set_buffers(disp, lvBuffer[0], lvBuffer[1], LVBUF,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  ui_init();

  lv_obj_set_style_text_color(ui_Screen1, lv_color_make(0x00, 0x00, 0x00),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  lv_timer_create(update_clock, 60000, NULL);
  delay(1000);
}

void loop() {
  lv_timer_handler(); // powinien być wywoływany bardzo często (np. co 10ms)
  delay(10);          // minimalny delay dla stability
}
