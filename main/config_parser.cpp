/**
 * @file config_parser.cpp
 * @brief Lightweight TOML parser for config.toml on the SD card.
 *
 * Supported format (subset of TOML):
 *
 *   [general]
 *   default_gif = /busy.gif
 *
 *   [busy]
 *   id = 1231
 *   gif = /busy.gif
 *
 *   [coffee]
 *   id = 1232
 *   gif = /coffee.gif
 *
 * - Lines starting with '#' are comments.
 * - Blank lines are ignored.
 * - Section names in [brackets] become command names.
 * - [general] is a reserved section for global settings.
 * - Values can optionally be quoted: gif = "/busy.gif"
 */

#include "config_parser.h"
#include <SD.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define CONFIG_FILE_PATH "/config.toml"
#define LINE_BUF_SIZE    128

/* ── helpers ────────────────────────────────────────────── */

/** Trim leading and trailing whitespace in-place, return pointer. */
static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/** Remove optional surrounding quotes from a value string. */
static void strip_quotes(char *s)
{
    size_t len = strlen(s);
    if (len >= 2 && ((s[0] == '"' && s[len-1] == '"') ||
                     (s[0] == '\'' && s[len-1] == '\''))) {
        memmove(s, s + 1, len - 2);
        s[len - 2] = '\0';
    }
}

/* ── public API ─────────────────────────────────────────── */

bool config_load(config_t *cfg)
{
    memset(cfg, 0, sizeof(config_t));
    cfg->count = 0;
    cfg->default_color = 0xFFFFFF;  /* white fallback */
    strncpy(cfg->default_gif, "/busy.gif", MAX_GIF_PATH_LEN); /* fallback */

    File file = SD.open(CONFIG_FILE_PATH, FILE_READ);
    if (!file) {
        printf("[config] Could not open %s\n", CONFIG_FILE_PATH);
        return false;
    }

    printf("[config] Parsing %s (%lu bytes)\n", CONFIG_FILE_PATH, (unsigned long)file.size());

    char line_buf[LINE_BUF_SIZE];
    bool in_general = false;              /* true when inside [general]  */
    int  current_cmd = -1;                /* index into cfg->commands[]  */

    while (file.available()) {
        /* Read one line */
        int len = 0;
        while (file.available() && len < LINE_BUF_SIZE - 1) {
            char c = file.read();
            if (c == '\n') break;
            if (c == '\r') continue;      /* skip CR */
            line_buf[len++] = c;
        }
        line_buf[len] = '\0';

        char *line = trim(line_buf);

        /* Skip empty lines and comments */
        if (*line == '\0' || *line == '#') continue;

        /* Section header: [name] */
        if (*line == '[') {
            char *end_bracket = strchr(line, ']');
            if (!end_bracket) continue;   /* malformed, skip */

            *end_bracket = '\0';
            char *section_name = trim(line + 1);

            if (strcasecmp(section_name, "general") == 0) {
                in_general = true;
                current_cmd = -1;
            } else {
                in_general = false;
                if (cfg->count < MAX_COMMANDS) {
                    current_cmd = cfg->count++;
                    memset(&cfg->commands[current_cmd], 0, sizeof(command_entry_t));
                    strncpy(cfg->commands[current_cmd].name, section_name, MAX_CMD_NAME_LEN - 1);
                    cfg->commands[current_cmd].color = cfg->default_color; /* inherit default */
                } else {
                    printf("[config] WARNING: max commands (%d) reached, ignoring [%s]\n",
                           MAX_COMMANDS, section_name);
                    current_cmd = -1;
                }
            }
            continue;
        }

        /* Key = Value */
        char *eq = strchr(line, '=');
        if (!eq) continue;   /* no '=', skip */

        *eq = '\0';
        char *key   = trim(line);
        char *value = trim(eq + 1);
        strip_quotes(value);

        if (in_general) {
            if (strcasecmp(key, "default_gif") == 0) {
                strncpy(cfg->default_gif, value, MAX_GIF_PATH_LEN - 1);
            }
            else if (strcasecmp(key, "color") == 0) {
                cfg->default_color = strtoul(value, NULL, 16);
            }
            /* Add more [general] keys here in the future */
        }
        else if (current_cmd >= 0) {
            if (strcasecmp(key, "id") == 0) {
                cfg->commands[current_cmd].id = atoi(value);
            }
            else if (strcasecmp(key, "gif") == 0) {
                strncpy(cfg->commands[current_cmd].gif, value, MAX_GIF_PATH_LEN - 1);
            }
            else if (strcasecmp(key, "text") == 0) {
                strncpy(cfg->commands[current_cmd].text, value, MAX_TEXT_LEN - 1);
            }
            else if (strcasecmp(key, "color") == 0) {
                cfg->commands[current_cmd].color = strtoul(value, NULL, 16);
            }
        }
    }

    file.close();
    printf("[config] Loaded %d command(s)\n", cfg->count);
    return cfg->count > 0;
}

const command_entry_t *config_find_by_id(const config_t *cfg, int id)
{
    for (int i = 0; i < cfg->count; i++) {
        if (cfg->commands[i].id == id) {
            return &cfg->commands[i];
        }
    }
    return NULL;
}

void config_print(const config_t *cfg)
{
    printf("[config] default_gif = %s  default_color = #%06X\n", cfg->default_gif, (unsigned int)cfg->default_color);
    printf("[config] Commands:\n");
    for (int i = 0; i < cfg->count; i++) {
        printf("  [%s]  id=%d  gif=%s  text=\"%s\"  color=#%06X\n",
               cfg->commands[i].name,
               cfg->commands[i].id,
               cfg->commands[i].gif,
               cfg->commands[i].text,
               (unsigned int)cfg->commands[i].color);
    }
}
