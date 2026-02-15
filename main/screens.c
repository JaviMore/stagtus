#include <string.h>
#include <stdio.h>
#include "screens.h"
#include "ui.h"

LV_FONT_DECLARE(game_14);
LV_FONT_DECLARE(game_20);
LV_FONT_DECLARE(game_24);
LV_FONT_DECLARE(game_34);
LV_FONT_DECLARE(game_38);
LV_FONT_DECLARE(game_40);
LV_FONT_DECLARE(game_48);

objects_t objects;

/* Current GIF path on the SD card (prefixed with drive letter for LVGL) */
static char current_gif_path[128] = "S:/busy.gif";

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 172, 320);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // label_top – fixed title at the top
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_top = obj;
            lv_label_set_text(obj, "STAGTUS");
            lv_obj_set_width(obj, 172);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), 0);
            lv_obj_set_style_text_font(obj, &game_20, 0);
            lv_obj_align(obj, LV_ALIGN_TOP_MID, 0, 12);
        }
        {
            // gif_container – use lv_gif_create for animated GIF support
            lv_obj_t *obj = lv_gif_create(parent_obj);
            objects.gif_container = obj;
            lv_obj_set_pos(obj, 0, 47);
            lv_obj_set_size(obj, 172, 172);
            lv_gif_set_src(obj, current_gif_path);
        }
        {
            // label_bottom – status text, configurable per command
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.label_bottom = obj;
            lv_label_set_text(obj, "");
            lv_obj_set_width(obj, 172);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), 0);
            lv_obj_set_style_text_font(obj, &game_40, 0);
            lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, -2);
        }
    }
    
    tick_screen_main();
}

void set_gif_src(const char *sd_path) {
    /* sd_path should be the file path on the SD card, e.g. "/busy.gif" or "/animations/walk.gif" */
    snprintf(current_gif_path, sizeof(current_gif_path), "S:%s", sd_path);
    if (objects.gif_container != NULL) {
        lv_gif_set_src(objects.gif_container, current_gif_path);
    }
}

/*
 * Available height for the bottom label:
 *   GIF ends at y = 47 + 172 = 219
 *   Screen bottom minus margin = 320 - 2 = 318
 *   Max label height = 318 - 219 = 99 px
 */
#define BOTTOM_LABEL_MAX_H  99

void set_bottom_text(const char *text, uint32_t color) {
    if (objects.label_bottom != NULL) {
        lv_label_set_text(objects.label_bottom, text);
        lv_obj_set_style_text_color(objects.label_bottom, lv_color_hex(color), 0);

        /* Single word = must fit in one line (width only).
         * Multiple words = may wrap, but must fit in available height. */
        int single_word = (strchr(text, ' ') == NULL);

        static const lv_font_t *fonts[] = { &game_48, &game_40, &game_38, &game_34, &game_24, &game_20, &game_14 };
        for (int i = 0; i < 7; i++) {
            lv_obj_set_style_text_font(objects.label_bottom, fonts[i], 0);
            lv_obj_update_layout(objects.label_bottom);

            if (single_word) {
                /* Check that the text doesn't wrap (height = 1 line) */
                lv_coord_t line_h = lv_font_get_line_height(fonts[i]);
                lv_coord_t content_h = lv_obj_get_content_height(objects.label_bottom);
                if (content_h <= line_h && content_h <= BOTTOM_LABEL_MAX_H) {
                    break;
                }
            } else {
                if (lv_obj_get_content_height(objects.label_bottom) <= BOTTOM_LABEL_MAX_H) {
                    break;
                }
            }
        }
    }
    if (objects.label_top != NULL) {
        lv_obj_set_style_text_color(objects.label_top, lv_color_hex(color), 0);
    }
}

void tick_screen_main() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
}
