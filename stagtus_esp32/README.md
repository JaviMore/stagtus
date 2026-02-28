# STAGTUS

Status display for ESP32-C6-LCD-1.47 — shows animated GIFs and status text controlled via serial commands, fully configurable through a TOML file on the SD card.

## Hardware

| Component | Details |
|-----------|---------|
| Board | [ESP32-C6-LCD-1.47](https://www.waveshare.com/wiki/ESP32-C6-LCD-1.47) |
| Display | ST7789, 172×320 px, SPI @ 80 MHz |
| SD Card | SPI @ 40 MHz, CS = GPIO 4 |
| Framework | Arduino (ESP32 board package 3.2.0) |

### Pin mapping

| Function | GPIO |
|----------|------|
| SPI MISO | 5 |
| SPI MOSI | 6 |
| SPI SCLK | 7 |
| LCD CS | 14 |
| LCD DC | 15 |
| LCD RST | 21 |
| Backlight | 22 |
| SD CS | 4 |

## Features

- Animated GIF playback from SD card via LVGL's GIF decoder
- Runtime GIF and text changes via serial commands
- Config-driven command system (TOML file on SD)
- Custom "Gameplay" font with auto-size: text scales down automatically to fit the available space
- Montserrat font as fallback for characters not included in the custom font
- Per-command configurable color applied to both title and status text

## Screen layout

```
┌──────────────────┐
│     STAGTUS      │  ← label_top (game_20, y=12)
│                  │
│   ┌──────────┐   │
│   │          │   │
│   │   GIF    │   │  ← 172×172 px (y=47)
│   │          │   │
│   └──────────┘   │
│                  │
│   STATUS TEXT    │  ← label_bottom (auto-sized, bottom-aligned)
└──────────────────┘
     172 × 320 px
```

## SD card structure

```
/
├── config.toml        ← Command configuration
├── busy.gif           ← GIF files referenced in config
├── coffee.gif
├── meeting.gif
└── ...
```

### GIF requirements

- **Dimensions**: 172×172 px (matches the GIF container)
- **Format**: Standard GIF87a/GIF89a with animation support
- Larger GIFs will work but may be cropped or slower to decode
- **Tool**: Use [ezgif.com](https://ezgif.com/) to resize, crop, or optimize your GIFs

## Configuration — `config.toml`

The file `/config.toml` on the SD card defines all serial commands. It uses a simplified TOML format.

### Structure

```toml
# General settings
[general]
default_gif = /busy.gif       # GIF shown on boot
color = FFFFFF                 # Default text color (hex RGB), applied when a command doesn't specify its own

# Command definitions — each section defines a command
[busy]
id = 1                         # Integer ID sent via serial to trigger this command
gif = /busy.gif                # GIF to display
text = BUSY                    # Text shown below the GIF
color = FF0000                 # Text color override (hex RGB, optional)

[coffee]
id = 2
gif = /coffee.gif
text = COFFEE
color = 8B4513

[meeting]
id = 3
gif = /meeting.gif
text = IN A MEETING             # Multi-word text will wrap and auto-size
# color is omitted → inherits from [general]
```

### Rules

| Rule | Details |
|------|---------|
| `[general]` | Reserved section for global settings. Not a command. |
| `id` | **Required.** Unique integer identifier for the command. |
| `gif` | Path on the SD card. Must start with `/`. |
| `text` | Text displayed on the bottom label. |
| `color` | Hex RGB color (without `#` or `0x`). Optional — inherits from `[general]` if omitted. |
| Comments | Lines starting with `#` are ignored. |
| Quotes | Values can optionally be quoted: `gif = "/busy.gif"` |
| Max commands | 16 |

## Serial protocol

Connect at **115200 baud**. Send one of the following followed by a newline (`\n`):

| Input | Action |
|-------|--------|
| `1` | Execute command with `id = 1` (changes GIF, text, and color) |
| `2` | Execute command with `id = 2` |
| `N` | Execute command with `id = N` |

### Example session

```
> 1
Command: busy (id=1)
> 2
Command: coffee (id=2)
> 3
Command: meeting (id=3)
```

## Font auto-sizing

The bottom status text automatically selects the largest font that fits without overlapping the GIF. Available sizes (tried in order):

**game_48 → game_40 → game_38 → game_34 → game_24 → game_20 → game_14**

- **Single word**: the font must fit in one line (no wrapping allowed).
- **Multiple words**: wrapping is allowed, but total height must not exceed the available 99px below the GIF.

All Gameplay fonts fall back to Montserrat at the same size for any missing characters.

## Project structure

```
main/
├── main.ino              # Entry point: setup, loop, serial dispatch
├── screens.c / .h        # UI: screen creation, set_gif_src(), set_bottom_text()
├── ui.c / .h             # Screen loading and tick
├── config_parser.cpp / .h # TOML parser for /config.toml
├── lv_sd_fs.cpp / .h     # LVGL filesystem driver bridging to Arduino SD
├── LVGL_Driver.cpp / .h  # LVGL display driver initialization
├── Display_ST7789.cpp / .h # ST7789 LCD low-level SPI driver
├── SD_Card.cpp / .h      # SD card initialization
├── game_14.c             # Gameplay font, 14px
├── game_20.c             # Gameplay font, 20px
├── game_24.c             # Gameplay font, 24px
├── game_34.c             # Gameplay font, 34px
├── game_38.c             # Gameplay font, 38px
├── game_40.c             # Gameplay font, 40px
└── game_48.c             # Gameplay font, 48px
libraries/
└── lvgl/                 # LVGL v8.x (source only, no demos/examples)
    ├── library.properties
    ├── LICENCE.txt
    ├── lvgl.h
    └── src/              # Core, draw, extra, font, hal, misc, widgets
```

## Building

1. Open `main/main.ino` in the Arduino IDE
2. Select board: **ESP32C6 Dev Module**
3. Set USB CDC On Boot = **Enabled** (to allow serial io after flashing)
4. Set Flash size: **4MB**
5. Set Partition Scheme: **No OTA (2MB APP/2MB FATFS)**
6. Ensure the `libraries/lvgl/` folder is detected (should auto-detect from the project structure)
7. Compile and upload

## Adding a new command

1. Create your GIF (172×172 px) and copy it to the SD card root
2. Add a new section to `/config.toml`:
   ```toml
   [lunch]
   id = 4
   gif = /lunch.gif
   text = LUNCH
   color = 00AA00
   ```
3. Send `4` via serial to activate it

## Adding a new font size

1. Go to https://lvgl.io/tools/fontconverter
2. Upload the `.ttf` file, select the desired size, range `0x20-0x7F`, Bpp = 4, format = LVGL
3. Save the `.c` file to `main/` (e.g. `game_28.c`)
4. Remove the `.static_bitmap = 0` line from the generated file (not compatible with LVGL v8.x)
5. Enable the matching Montserrat fallback in `lv_conf.h`: `#define LV_FONT_MONTSERRAT_28 1`
6. Add it to `LV_FONT_CUSTOM_DECLARE` in `lv_conf.h`
7. Add `LV_FONT_DECLARE(game_28);` in `screens.c` and insert `&game_28` in the auto-size array

## License

LVGL is licensed under MIT. See [libraries/lvgl/LICENCE.txt](libraries/lvgl/LICENCE.txt).