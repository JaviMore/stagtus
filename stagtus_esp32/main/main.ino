#include "SD_Card.h"
#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "ui.h"
#include "screens.h"
#include "lv_sd_fs.h"
#include "config_parser.h"
#include <esp32-hal-rgb-led.h>

static config_t app_config;

#define RGB_CONTROL_PIN 8

static void set_rgb_led_color(uint32_t color)
{
  uint8_t red = (color >> 16) & 0xFF;
  uint8_t green = (color >> 8) & 0xFF;
  uint8_t blue = color & 0xFF;

  /* Most ESP32C6 onboard RGB LEDs use GRB channel order. */
  neopixelWrite(RGB_CONTROL_PIN, green, red, blue);
}

void setup()
{
  Serial.begin(115200);
  LCD_Init();
  Lvgl_Init();
  SD_Init();
  lv_sd_fs_init();

  /* Load command config from /config.toml on the SD card */
  if (config_load(&app_config)) {
    config_print(&app_config);
  } else {
    printf("[main] WARNING: config.toml not found or empty, using defaults\n");
  }

  /* Set the default GIF before creating screens */
  set_gif_src(app_config.default_gif);

  ui_init();

  set_bottom_text("Take my money!", app_config.default_color);
  set_rgb_led_color(app_config.default_led_color);
}

void loop()
{
  lv_timer_handler();
  if (Serial.available() > 0) {

    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    /* Try to parse as integer command */
    int cmd = input.toInt();

    if (cmd != 0) {
      const command_entry_t *entry = config_find_by_id(&app_config, cmd);
      if (entry) {
        Serial.printf("Command: %s (id=%d)\n", entry->name, entry->id);
        set_gif_src(entry->gif);
        set_bottom_text(entry->text, entry->color);
        set_rgb_led_color(entry->led_color);
      } else {
        Serial.printf("Unknown command id: %d\n", cmd);
      }
    } else {
      Serial.printf("Unrecognized input: %s\n", input.c_str());
    }
  }
}
