# 😎 Stagtus

A physical status indicator you can place on your desk. Change your status from the system tray on your computer and watch it update in real time on a tiny LCD screen connected via USB serial.

The project has two halves that work together:

| Component | Description | Location |
|-----------|-------------|----------|
| **stagtus_desktop** | System tray app (Go + Fyne) that shows a menu of statuses and sends the selected command over serial. | [`stagtus_desktop/`](stagtus_desktop/) |
| **stagtus_esp32** | Arduino firmware for the ESP32-C6-LCD-1.47 that receives commands and displays animated GIFs with status text on a 172×320 ST7789 screen using LVGL. | [`stagtus_esp32/`](stagtus_esp32/) |

## How it works

```
┌──────────────┐   serial (USB)   ┌──────────────────┐
│  Desktop app │ ───────────────► │  ESP32-C6-LCD    │
│  system tray │   command id     │  172×320 display  │
└──────────────┘                  └──────────────────┘
```

1. You click a status in the system tray (e.g. *Busy*, *Coffee*, *Focus*).
2. The desktop app sends the command `id` over USB serial at 115 200 baud.
3. The ESP32 receives the id, looks it up in its own `config.toml` on the SD card, and switches the GIF and label on screen.

## Shared configuration

Both projects read the same `config.toml` format. Each section defines a status command with fields consumed by one or both sides:

```toml
[busy]
emoji = "busy.png"     # desktop tray icon
id    = "1231"         # serial command (both)
gif   = "/busy.gif"    # ESP32 GIF path
text  = "Busy"         # display text (both)
color = "F22F07"       # ESP32 text color
```

See the README in each subfolder for the full field reference and setup instructions.

## Repository structure

```
stagtus/
├── stagtus_desktop/    # Desktop system tray app (Go)
├── stagtus_esp32/      # ESP32 Arduino firmware (C/C++)
├── gifs/               # Sample GIF assets (172×172 px)
│   ├── busy.gif
│   ├── call.gif
│   ├── coffe.gif
│   ├── default.gif
│   ├── focus.gif
│   └── lunch.gif
├── .github/
│   └── workflows/
│       └── release.yml # CI: builds desktop binaries and creates a GitHub Release
└── README.md           # this file
```

## Quick start

### Desktop app

```bash
cd stagtus_desktop
go build -ldflags="-s -w" -o stagtus
./stagtus
```

See [stagtus_desktop/README.md](stagtus_desktop/README.md) for platform-specific instructions (Windows icon embedding, Linux dependencies, etc.).

### ESP32 firmware

1. Copy `config.toml` and your GIFs to an SD card.
2. Open `stagtus_esp32/main/main.ino` in the Arduino IDE.
3. Select **ESP32C6 Dev Module**, compile, and upload.

See [stagtus_esp32/README.md](stagtus_esp32/README.md) for full hardware wiring, build settings, and serial protocol details.

## Releases

Desktop binaries (Windows, Linux, macOS) are published automatically via GitHub Actions. Go to the [Releases](../../releases) page to download the latest build — each zip contains the binary and a sample `config.toml`.

## Contributing

The easiest way to contribute is by adding new GIF animations. Just open a pull request with your GIF(s) in the `gifs/` folder:

1. Create your GIF at **172×172 px** (the exact size of the display's GIF container).
2. Drop it into `gifs/`.
3. Open a PR — that's it!

[ezgif.com](https://ezgif.com/) is a handy tool for resizing, cropping, and optimizing GIFs.
