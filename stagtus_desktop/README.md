# Stagtus Desktop — System Tray Status App

A lightweight system tray application that lets you toggle status modes (e.g. **Busy**, **Coffee**, **Focus**) and sends the corresponding command to an ESP32 device over serial.

## Requirements

- Go 1.25.6 or later
- CGO enabled (`go env CGO_ENABLED` should be `1`)
- **Windows**: Windows 10/11 (64‑bit)
- **Linux**: `libgl1-mesa-dev`, `xorg-dev`
- **macOS**: Xcode Command Line Tools

## Build

### Windows

```powershell
# First time only — install resource tool
go install github.com/akavel/rsrc@latest

# Generate Windows resources containing the icon
rsrc -ico stagtus.ico -o rsrc.syso

# Build the binary (no console window)
go build -ldflags="-H=windowsgui -s -w" -o stagtus.exe
```

### Linux / macOS

```bash
go build -ldflags="-s -w" -o stagtus
```

### Build flags explained

| Flag | Purpose |
|------|---------|
| `-H=windowsgui` | (Windows only) produces a GUI app without a console window |
| `-s -w` | Strip symbol/debug info to reduce binary size |

## Run

```bash
./stagtus        # Linux / macOS
.\stagtus.exe    # Windows
```

The program will start without showing a window and will reside in the system tray.

## Configuration — `config.toml`

The desktop program reads a `config.toml` file located either in the same
folder as the executable or in the current working directory. This TOML file
defines the set of tray commands that the app will expose and is the only
method of customizing behavior without recompiling.

The format mirrors the command system used by the ESP32 version; both
projects share the same `config.toml` schema. Example:

```toml
[general]
default_gif = "/default.gif"   # used by ESP32; ignored by desktop
color = "FFFFFF"               # used by ESP32; ignored by desktop

[busy]
emoji = "busy.png"             # tray icon (from the embedded images table below)
id = "1231"                    # sent over serial when selected
gif = "/busy.gif"              # used by ESP32
text = "Busy"                  # label shown in the tray submenu
color = "F22F07"               # used by ESP32

[coffee]
emoji = "coffee.png"
id = "1232"
gif = "/coffee.gif"
text = "Coffee"
color = "723B04"
```

### Field reference

| Field | Required | Used by | Description |
|-------|----------|---------|-------------|
| `emoji` | No | Desktop | Filename of the embedded PNG used as the tray icon when this command is active. See the table below for available images. |
| `id` | Yes | Both | Unique string identifier sent via serial when the user selects this item. |
| `text` | Yes | Both | Display string for the menu item (desktop) / bottom label (ESP32). |
| `gif` | No | ESP32 | Path to the GIF on the SD card. Ignored by the desktop app. |
| `color` | No | ESP32 | Hex RGB color for text. Ignored by the desktop app. |

### Embedded images

The following PNG files are compiled into the binary via `//go:embed` and can
be referenced in the `emoji` field of `config.toml`:

| Filename | Description |
|----------|-------------|
| `stagtus.png` | 😎 Default tray icon (app logo) |
| `busy.png` | 🔴 Busy / Do not disturb |
| `coffee.png` | ☕ Coffee break |
| `focus.png` | 🎯 Focus mode |
| `phone.png` | 📞 On a call / Meeting |
| `lunch.png` | 🍽️ Lunch time |
| `clock.png` | 🕐 Clock / Away |
| `cigarette.png` | 🚬 Smoke break |
| `game.png` | 🎮 Gaming |

> If `emoji` is omitted or the filename is not found, the app falls back to
> `stagtus.png`.

## Project structure

```
stagtus_desktop/
├── stagtus.go          # main application
├── stagtus.ico         # icon embedded in the Windows executable
├── rsrc.syso           # auto-generated Windows resource (contains stagtus.ico)
├── go.mod              # module dependencies
├── config.toml         # runtime command configuration
├── images/             # tray icons (embedded at compile time)
│   ├── stagtus.png
│   ├── busy.png
│   ├── cigarette.png
│   ├── clock.png
│   ├── coffee.png
│   ├── focus.png
│   ├── game.png
│   ├── lunch.png
│   └── phone.png
└── README.md           # this document
```

## Notes

- `rsrc.syso` is generated automatically and contains the embedded icon.
- Do **not** use `fyne package`; the resulting binaries are incompatible with Go 1.25+.
- If you change the icon, regenerate `rsrc.syso` before rebuilding.

