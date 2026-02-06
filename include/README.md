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
   - Update the `Settings.h` file with your Wi-Fi SSID and password.

3. **Upload the Code**:
   - Connect the ESP8266 to your computer using a USB cable.
   - Use the Arduino IDE to upload the code to the ESP8266.

### Usage

- **Time Display**: The clock displays the current time on the WS2812B LED strip, with smooth brightness transitions based on ambient light levels.
- **Nighttime Mode**: During nighttime, the digits on the clock turn red for better visibility.
- **Brightness Adjustment**: The brightness of the LED strip is automatically adjusted to ensure the clock is readable under various lighting conditions.
## Planned Features by Version

