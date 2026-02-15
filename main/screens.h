#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *label_top;
    lv_obj_t *gif_container;
    lv_obj_t *label_bottom;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
};

void create_screen_main();
void tick_screen_main();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

/**
 * @brief Change the GIF displayed on the main screen at runtime.
 * @param sd_path Path on the SD card, e.g. "/busy.gif" or "/animations/walk.gif"
 */
void set_gif_src(const char *sd_path);

/**
 * @brief Change the bottom label text and color at runtime.
 * @param text  The text to display at the bottom of the screen.
 * @param color Color as 0xRRGGBB (e.g. 0xFF0000 for red).
 */
void set_bottom_text(const char *text, uint32_t color);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/