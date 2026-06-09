#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of commands that can be defined in config.toml */
#define MAX_COMMANDS     16

/* Maximum string lengths */
#define MAX_CMD_NAME_LEN 32
#define MAX_GIF_PATH_LEN 64
#define MAX_TEXT_LEN     48

/**
 * A single command entry parsed from config.toml
 *
 * Example TOML section:
 *   [busy]
 *   id = 1231
 *   gif = /busy.gif
 *   text = Do not disturb
 *   color = FF0000
 */
typedef struct {
    char name[MAX_CMD_NAME_LEN];   /* Section name, e.g. "busy"       */
    int  id;                       /* Integer sent via serial          */
    char gif[MAX_GIF_PATH_LEN];    /* Path on SD card, e.g. "/busy.gif" */
    char text[MAX_TEXT_LEN];       /* Bottom label text                */
    uint32_t color;                /* Text color as 0xRRGGBB           */
    uint32_t led_color;            /* LED color as 0xRRGGBB            */
} command_entry_t;

/**
 * Holds all parsed commands and the default GIF to show on boot.
 */
typedef struct {
    char default_gif[MAX_GIF_PATH_LEN];  /* From [general] default_gif  */
    uint32_t default_color;              /* From [general] color        */
    uint32_t default_led_color;          /* From [general] led          */
    command_entry_t commands[MAX_COMMANDS];
    int count;                           /* Number of commands loaded   */
} config_t;

/**
 * @brief Parse /config.toml from the SD card.
 * @param cfg  Pointer to config_t struct to fill.
 * @return true if parsed successfully, false on error.
 */
bool config_load(config_t *cfg);

/**
 * @brief Find a command entry by its integer id.
 * @param cfg  Pointer to the loaded config.
 * @param id   The integer command id received via serial.
 * @return Pointer to the matching command_entry_t, or NULL if not found.
 */
const command_entry_t *config_find_by_id(const config_t *cfg, int id);

/**
 * @brief Print all loaded commands to serial (for debugging).
 */
void config_print(const config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_PARSER_H */
