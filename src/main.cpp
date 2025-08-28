#include "lvgl.h"
#include "ui/ui.h"
#include <Arduino.h>
#include <GxEPD2_BW.h>

#define ENABLE_GxEPD2_GFX 0

// Sterownik dla Twojego ekranu / panelu
GxEPD2_BW<GxEPD2_579_GDEY0579T93, GxEPD2_579_GDEY0579T93::HEIGHT> display(GxEPD2_579_GDEY0579T93(/*CS=5*/ 45, /*DC=*/46, /*RST=*/47, /*BUSY=*/48)); // GDEY0579T93 792x272, SSD1683 (FPC-E004 22.04.13)

#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_579_GDEY0579T93 // GDEY0579T93 792x272, SSD1683 (FPC-E004 22.04.13)

#define SCR_WIDTH 792
#define SCR_HEIGHT 272

#define LVBUF                                                                  \
  ((SCR_WIDTH * SCR_HEIGHT / 8) +                                              \
   8) // Bufor LVGL (8-bit, rozmiar nieskompresowany)

// Flaga gotowości obrazu - do synchronizacji wyświetlania
bool screen = false;

static lv_disp_t *lvDisplay;
static uint8_t lvBuffer[2][LVBUF];

// Bufor statyczny dla konwersji do 1-bit
static uint8_t converted_buf[(SCR_WIDTH * SCR_HEIGHT) / 8 + 1];

// Funkcja konwersji i flush dla e-ink
void my_disp_flush(lv_disp_t *disp, const lv_area_t *area, uint8_t *data) {
  int16_t width = area->x2 - area->x1 + 1;
  int16_t height = area->y2 - area->y1 + 1;

  Serial.print("FLUSH x1=");
  Serial.print(area->x1);
  Serial.print(" y1=");
  Serial.print(area->y1);
  Serial.print(" x2=");
  Serial.print(area->x2);
  Serial.print(" y2=");
  Serial.println(area->y2);

  int buf_size = width * height;

  Serial.print("Data sample: ");
  for (int i = 0; i < 16 && i < buf_size; i++) {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Zeruj bufor na start
  memset(converted_buf, 0x00, sizeof(converted_buf));

  int converted_index = 0;
  uint8_t bit_mask = 0x80;

  for (int i = 0; i < buf_size; i++) {
    uint8_t pixel = data[i]; // wartosc 0..255 (LVGL A8)
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

  lv_disp_flush_ready(disp);

  screen = (area->x1 + width == SCR_WIDTH) && (area->y1 + height == SCR_HEIGHT);
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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // czekaj aż serial będzie gotowy (np. dla Arduino Leonardo, ESP32 itp.)
  }

  pinMode(7, OUTPUT);
  digitalWrite(7, HIGH); // zasil ekran

  epd_setup();

  lv_init();

  lv_tick_set_cb(my_tick);

  lvDisplay = lv_display_create(SCR_WIDTH, SCR_HEIGHT);
  lv_display_set_flush_cb(lvDisplay, my_disp_flush);
  lv_display_set_buffers(lvDisplay, lvBuffer[0], lvBuffer[1], LVBUF,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  ui_init();

  lv_obj_set_style_text_color(ui_Screen1, lv_color_make(0x00, 0x00, 0x00),
                              LV_PART_MAIN | LV_STATE_DEFAULT);

  // czekaj na pierwszy flush - debug często blokuje użycie while w setup
  while (!screen) {
    delay(100);
    lv_timer_handler();
  }

  lv_deinit();
  digitalWrite(7, LOW); // wyłącz zasilanie ekranu
}

void loop() {
  // E-ink nie wymaga ciągłego odświeżania
  delay(5000);
  // opcjonalnie lv_timer_handler() można tu wywołać, jeśli chcesz wymusić
  // aktualizację
}
