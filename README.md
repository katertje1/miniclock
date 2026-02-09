# MiniClock

Wi-Fi connected ESP8266 LED clock (WS2812B) with:
- clock display
- brightness control (day/night offsets)
- multiple modes (Clock, Stopwatch, Love, Rainbow, Food)
- web UI + multi-clock controller
- diagnostics badge + crash details popup
- OTA and USB upload support

## Quick Start

### 1) Build
```bash
pio run
```

### 2) Upload (USB)
```bash
pio run -e nodemcuv2 -t upload --upload-port /dev/cu.usbserial-xxx
```

### 3) Upload (OTA)
```bash
pio run -e nodemcuv2_ota -t upload --upload-port <device>.local
```

### 4) Upload helper script
```bash
tools/upload.py
```

## Web Interfaces

- Device web UI: `http://<device>.local/`
- Controller page: `http://<device>.local/controller`
- Both pages are served from LittleFS files in `data/` (`index.html`, `controller.html`).
- After changing `data/` files, upload filesystem (`uploadfs`).

Diagnostics:
- `/` shows a diagnostics status badge (green/red/unknown), click for full JSON.
- `/` uses lightweight diagnostics polling (`/getDiagnosticsSummary`) and fetches full diagnostics on click.
- `/controller` shows diagnostics per clock; red cells are clickable for detailed JSON.

## Project Structure

- Firmware source: `src/`
- Controller assets (LittleFS): `data/`
- PlatformIO config: `platformio.ini`
- Upload tools: `tools/`
- Extended docs: `include/README.md`
- Release notes: `include/ReleaseNotes.md`

## Local Secrets

Local/private settings are intentionally not tracked:
- `src/Settings.cpp`
- `tools/upload.local.json`
