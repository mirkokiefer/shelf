# 📋 Shelf — Physical Daslab Scene Widget

A physical e-ink dashboard powered by [Daslab](https://daslab.xyz). Your Daslab scene rendered on a **Xteink X4** e-paper display, updated in real-time over WiFi.

## What is this?

**Shelf** turns a $69 Xteink X4 e-reader into a beautiful, always-on dashboard for your Daslab scene. The server renders your scene data (GitHub stats, deploy status, device health, quotes) as a 480×800 pixel image using [Satori](https://github.com/vercel/satori), and the Xteink fetches it over WiFi every 60 seconds.

## Architecture

```
┌─────────────────┐        HTTP GET /eink.png       ┌──────────────────┐
│  Xteink X4      │ ◄──────────────────────────────  │  Bun + Hono      │
│  (E-Ink 480×800)│        every 60 seconds          │  Server          │
│                 │                                   │                  │
│  • WiFi         │   Satori JSX → SVG → PNG          │  • Renders image │
│  • ESP32-C3     │                                   │  • REST API      │
│  • Buttons      │        POST /api/data             │  • Web dashboard │
│  • 650mAh       │ ─────────────────────────────►   │                  │
└─────────────────┘        (mode changes)            └──────────────────┘
```

## Display Modes

| Mode | What it shows |
|------|--------------|
| **Dashboard** | Time, weather, GitHub repo stars, deploy status, device health, quote |
| **Quote** | Large centered inspirational quote with timestamp |
| **Status** | System status with device list and AI vision caption |

## 🚀 Quick Start

### 1. Start the server

```bash
cd shelf
bun install
bun run src/index.ts
```

Server runs on `http://localhost:3001`. Visit it to see the web dashboard with e-ink preview.

### 2. Flash the Xteink X4

Edit `firmware/xteink-shelf/include/config.h` with your WiFi credentials and server URL:

```cpp
#define WIFI_SSID "your-wifi"
#define WIFI_PASS "your-password"
#define SHELF_SERVER_URL "http://192.168.1.100:3001"
```

Then flash via PlatformIO:

```bash
cd firmware/xteink-shelf
git submodule add https://github.com/open-x4-epaper/community-sdk.git open-x4-sdk
pio run --target upload
```

Or use the [CrossPoint Flash Tools](https://crosspoint-reader.github.io/flash-tools/) web flasher.

### 3. Physical buttons

- **Page Forward** → Next display mode
- **Page Back** → Previous display mode  
- **OK** → Force refresh

## API

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Web dashboard with e-ink preview |
| `/eink.png` | GET | 480×800 PNG image for the e-ink display |
| `/api/data` | GET | Current scene data as JSON |
| `/api/data` | POST | Update scene data (partial merge) |
| `/health` | GET | Server health check |

## Phases

This is part of a 3-phase demo:

1. **📋 Shelf** (this) — Physical dashboard widget on Xteink X4
2. **🪞 Mirror** — Add ESP32-S3 camera, Claude Vision narrates your scene onto the e-ink
3. **🎬 Teleprompter** — Camera analyzes your framing during recording, coaching tips on e-ink

## Hardware

| Device | Price | Role |
|--------|-------|------|
| Xteink X4 | $69 | E-ink display |
| XIAO ESP32-S3 Sense | $13 | Camera (Phase 2) |

## Stack

- **Server**: Bun + Hono + Satori + @resvg/resvg-js
- **Firmware**: PlatformIO + Arduino + open-x4-sdk
- **Orchestration**: Daslab

## License

MIT
