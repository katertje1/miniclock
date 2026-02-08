### Project Overview

This project is a Wi-Fi-connected clock that uses an **ESP8266** microcontroller to display the time on a **WS2812B LED strip**. The clock automatically adjusts its brightness based on ambient light levels and changes the display color during nighttime to improve visibility.

### Hardware Description

**Components:**
- **ESP8266**: The main microcontroller used to control the LED strip and handle Wi-Fi connectivity.
- **WS2812B LED Strip**: Individually addressable RGB LEDs used to display the time.
- **Light Sensor (e.g., LDR or Photodiode)**: Used to measure ambient light levels to adjust the brightness of the LED strip.
- **Power Supply**: A 5V power supply is used to power the LED strip and the ESP8266.

**Connections:**
- **ESP8266 Pin D6**: Connected to the data input of the WS2812B LED strip.
- **ESP8266 Pin A0**: Connected to the output of the light sensor. This analog pin reads the ambient light level.
- **5V and GND**: Power connections for both the LED strip and ESP8266. Ensure the ground is common between the ESP8266, the light sensor, and the LED strip.

### Software Setup

1. **Install Required Libraries**:
   - Install the **Adafruit NeoPixel** library to control the WS2812B LED strip.
   - Install the **ESP8266WiFi** library for Wi-Fi connectivity.
   - Install the **NTPClient** library for time synchronization with NTP servers.

2. **Configure Wi-Fi Settings**:
   - Update `src/Settings.cpp`. Each clock in `clockConfigs` now has its own `ssid` and `password`.

3. **Upload the Code**:
   - Connect the ESP8266 to your computer using a USB cable.
   - Use PlatformIO (`pio run -t upload`) to upload.

4. **OTA Upload (optional)**:
   - Ensure the device is on Wi‑Fi.
   - OTA uses mDNS by default: `<deviceName>.local` (set via `currentConfig.deviceName`).
   - Upload via OTA with: `pio run -e nodemcuv2_ota -t upload`
   - If mDNS fails, use the IP address: `pio run -e nodemcuv2_ota -t upload --upload-port 192.168.1.x`

5. **Upload Helper (no IPs in git)**:
   - Run: `tools/upload.py` and select a clock + method.
   - The clock list is read from `src/Settings.cpp` every run.
   - Optional: create `tools/upload.local.json` if you want per‑clock `ota_host` overrides.
   - `tools/ports.py` prints available `/dev/cu.*` ports.
   - The helper can upload firmware, filesystem, or both (useful for `/controller`).

6. **Controller Page**:
   - The controller UI is served from `data/controller.html` at `/controller`.
   - After editing `data/controller.html`, upload the filesystem with: `pio run -t uploadfs` or `pio run -e nodemcuv2_ota -t uploadfs --upload-port <ip>`.
   - The controller uses `/getClockList` and `/getStatus`, and only shows reachable clocks.
   - Hostnames are generated as `<clockName>.local` with lowercase names (hyphen/underscore-safe).
   - The current device is auto-selected in the dropdown when opening `/controller` on that device.
   - Use **Show Probe Targets** to inspect which hosts are being probed and whether they are reachable.
   - Controller supports stopwatch commands for selected clock(s): `Set Stopwatch Mode`, `Start`, `Stop`, `Reset`, `Add 1 minute`.
   - Stopwatch controls are only shown when one clock is selected and that clock reports `Stopwatch Mode`.
   - Controller now shows a diagnostics status cell per clock (from `/getDiagnostics`):
     - green for normal/system restart
     - red for crash-like signals (exception/watchdog/fatal/etc.)
   - When a diagnostics cell is red, click it to open a popup with full diagnostics JSON and copy it.

7. **Diagnostics Endpoint**:
   - Use `/getDiagnostics` to inspect runtime and reset state without USB serial.
   - Response includes software version and (when time is synced) an estimated reset timestamp.
   - Example: `curl http://<deviceName>.local/getDiagnostics`

8. **Device Page Diagnostics**:
   - The root page (`/`) shows a diagnostics badge.
   - Badge is color-coded (green/red/unknown).
   - Click badge to open popup with full diagnostics JSON and copy button.

### Usage

- **Time Display**: The clock displays the current time on the WS2812B LED strip, with smooth brightness transitions based on ambient light levels.
- **Nighttime Mode**: During nighttime, the digits on the clock turn red for better visibility.
- **Brightness Adjustment**: The brightness of the LED strip is automatically adjusted to ensure the clock is readable under various lighting conditions.
- **Mode Rendering**: Rainbow/Food modes are rendered non-blocking so OTA/webserver stay responsive.
- **Stopwatch Mode**:
  - Countdown in `MM:SS` on the LED display.
  - Controls via web UI and controller (`start/stop/reset/add minute`).
  - When timer reaches `00:00`, display blinks 5 times, then stays on `00:00`.
- **Wi‑Fi Robustness**: The device retries Wi‑Fi connection in the main loop when disconnected.
## Planned Features by Version
