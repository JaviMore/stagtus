# 😎 Stagtus

A physical status indicator you can place on your desk. Open the web app in your browser, pick a status, and watch it update in real time on a tiny LCD screen connected via USB serial.

| Component | Description | Location |
|-----------|-------------|----------|
| **stagtus_web** | Browser-based controller (HTML + CSS + JS). Loads your `config.toml`, connects to the ESP32 via the Web Serial API, and sends commands on click. | [`stagtus_web/`](stagtus_web/) |
| **stagtus_esp32** | Arduino firmware for the ESP32-C6-LCD-1.47. Receives commands over USB serial and displays animated GIFs with status text on a 172×320 ST7789 screen using LVGL. | [`stagtus_esp32/`](stagtus_esp32/) |

## How it works

```
┌──────────────┐   serial (USB)   ┌──────────────────┐
│   Web app    │ ───────────────► │  ESP32-C6-LCD    │
│  (browser)   │   command id     │  172×320 display  │
└──────────────┘                  └──────────────────┘
```

1. Open the web app and click **Load config.toml** to load your configuration.
2. Click **Connect** and select the ESP32 from the serial port picker.
3. Click any status button — the app sends the command `id` over USB serial at 115 200 baud.
4. The ESP32 receives the id, looks it up in its own `config.toml` on the SD card, and switches the GIF and label on screen.

## Configuration

Both the web app and the ESP32 read the same `config.toml` format. Each section defines one status:

```toml
[general]
default_gif = "/default.gif"
color = "FFFFFF"
led   = "FFFFFF"

[busy]
emoji = "🔥"        # emoji shown in the web app
id    = "1231"      # serial command sent when selected
gif   = "/busy.gif" # GIF displayed on the ESP32
text  = "Busy"      # label shown in both web and ESP32
color = "F22F07"    # ESP32 text colour (hex RGB)
led   = "F22F07"    # ESP32 LED colour (hex RGB)
```

### Field reference

| Field | Used by | Description |
|-------|---------|-------------|
| `emoji` | Web | Emoji character shown on the status button. |
| `id` | Both | Unique string sent over serial when the status is selected. |
| `text` | Both | Label shown on the web button and on the ESP32 screen. |
| `gif` | ESP32 | Path to the GIF on the SD card. |
| `color` | ESP32 | Hex RGB colour for the on-screen text. |
| `led` | ESP32 | Hex RGB colour for the LED. |

### Adding a new status

Add a section to `config.toml` — no code changes needed:

```toml
[gaming]
emoji = "🎮"
id    = "1236"
gif   = "/gaming.gif"
text  = "Gaming"
color = "00C853"
led   = "00C853"
```

Then reload the config in the web app using **Load new config.toml**.

## Web app

### Use online (GitHub Pages)

The web app is deployed automatically from `stagtus_web/` on every push to `main`. Enable it once in your repository:

1. Go to **Settings → Pages**.
2. Set the source to **GitHub Actions**.
3. Push any change to `stagtus_web/` (or trigger `deploy-web.yml` manually).

### Use offline

Open `stagtus_web/index.html` directly in Chrome or Edge — no server needed.

> **Browser requirement:** [Chrome or Edge](https://developer.chrome.com/docs/capabilities/serial) — the Web Serial API is not available in Firefox or Safari.

### How to use

1. Open the web app (online or the local `index.html`).
2. Click **📂 Load config.toml** and select your file. The config is cached in `localStorage` — on the next visit it loads automatically.
3. Click **Connect** and pick the ESP32 from the browser's port picker.
4. Click any status button. If you already had a status selected when you connect, the command is sent automatically.

To load a different config, click **📂 Load new config.toml** in the config bar.

## ESP32 firmware

1. Copy `config.toml` and your GIFs to an SD card.
2. Open `stagtus_esp32/main/main.ino` in the Arduino IDE.
3. Select **ESP32C6 Dev Module**, compile, and upload.

See [stagtus_esp32/README.md](stagtus_esp32/README.md) for full hardware wiring, build settings, and serial protocol details.

## Repository structure

```
stagtus/
├── stagtus_web/        # Browser-based controller
│   ├── index.html
│   ├── app.js
│   ├── style.css
│   ├── config.toml     # Sample configuration
│   └── stagtus.ico     # Favicon
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
│       └── deploy-web.yml  # CI: deploys stagtus_web/ to GitHub Pages
└── README.md
```

## Contributing

The easiest way to contribute is by adding new GIF animations. Open a pull request with your GIF(s) in the `gifs/` folder:

1. Create your GIF at **172×172 px** (the exact size of the display's GIF container).
2. Drop it into `gifs/`.
3. Open a PR — that's it!

[ezgif.com](https://ezgif.com/) is a handy tool for resizing, cropping, and optimizing GIFs.
