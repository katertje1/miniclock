#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include "Settings.h"
#include "ClockDisplay.h"
#include "WebServer.h"
#include "SunriseSunset.h"

// Function Declarations
void connectToWiFi();

unsigned long debugPreviousMillis = 0;
const long debugInterval = 10000;  // Interval for debug messages

String lastSunriseSunsetCalcDate = "";

unsigned long lastDSTCheck = 0;
const unsigned long DST_UPDATE_INTERVAL = 3600000;  // 1 hour

int currentOffset = 0;

//#define TIMEZONE_OFFSET 2  // Central European Summer Time (CEST) is UTC+2

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started");

  currentConfig = clockConfigs[selectedClock];
  Serial.print("Using device: ");
  Serial.println(currentConfig.deviceName);
  dayTimeBrightnessOffset = currentConfig.dayTimeBrightnessOffset;
  nightTimeBrightnessOffset = currentConfig.nightTimeBrightnessOffset;
  hourColor = currentConfig.DEFAULT_HOUR_COLOR;
  minuteColor = currentConfig.DEFAULT_MINUTE_COLOR;
  colonColor = currentConfig.DEFAULT_COLON_COLOR;

  initClockDisplay();
  connectToWiFi();

timeClient.begin();
timeClient.update();  // Force NTP sync before checking DST
currentOffset = getDSTOffset();
timeClient.setTimeOffset(currentOffset);
  initWebServer();

  Serial.println("Setup completed");
}

void loop() {
  timeClient.update();

  if (millis() - lastDSTCheck > DST_UPDATE_INTERVAL) {
    int newOffset = getDSTOffset();
    Serial.print("DST Offset returned: ");
    Serial.println(newOffset);  // Should be 7200 during DST
    if (newOffset != currentOffset) {
      currentOffset = newOffset;
      timeClient.setTimeOffset(currentOffset);
      timeClient.update();
    }
    lastDSTCheck = millis();
  }
  time_t now = timeClient.getEpochTime();
  struct tm* nowInfo = localtime(&now);

  if (currentMode == CLOCK_MODE) {
    updateClockDisplay();
  }
  handleWebRequests();

  char currentDate[11];  // "YYYY-MM-DD"
  strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", nowInfo);
  String today = String(currentDate);

  // Only recalculate if the sunrise/sunset hasn't already been calculated for today
  if (today != lastSunriseSunsetCalcDate) {
    Serial.println("Day has changed. Recalculating sunrise and sunset times...");
    int year = nowInfo->tm_year + 1900;
    int month = nowInfo->tm_mon + 1;
    int day = nowInfo->tm_mday;

    calculateSunriseSunset(true, year, month, day);
    calculateSunriseSunset(false, year, month, day);
    lastSunriseSunsetCalcDate = today;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - debugPreviousMillis >= debugInterval) {
    debugPreviousMillis = currentMillis;

    Serial.println("Device :              " + String(currentConfig.deviceName));
    Serial.println("IP :                  " + WiFi.localIP().toString());

    char formattedTime[20];
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", nowInfo);

    Serial.print("Current Time: ");
    Serial.println(formattedTime);  // Displays the current time in local format

    Serial.print("Current clock mode is: ");
    Serial.println(getClockModeString(currentMode));

    bool isDaytime = isDayTime();  // Determine if it's daytime or nighttime
    Serial.print("Current time of day is: ");
    Serial.println(isDaytime ? "Daytime" : "Nighttime");

    // Display sunrise and sunset times after timezone and DST adjustments
    Serial.print("Sunrise time: ");
    Serial.print(ctime(&storedSunriseTime));  // Local sunrise time
    Serial.print("Sunset time: ");
    Serial.print(ctime(&storedSunsetTime));  // Local sunset time

    int hours = getHours();
    int minutes = getMinutes();

    Serial.print("Displaying Time: ");
    Serial.print(hours / 10);
    Serial.print(hours % 10);
    Serial.print(":");
    Serial.print(minutes / 10);
    Serial.println(minutes % 10);

    switch (WiFi.status()) {
      case WL_IDLE_STATUS:
        Serial.println("Wifi state: WL_IDLE_STATUS (0): Wi-Fi is in the idle state.");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("Wifi state: WL_NO_SSID_AVAIL (1): No SSID are available.");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("Wifi state: WL_SCAN_COMPLETED (2): Scan for networks is complete.");
        break;
      case WL_CONNECTED:
        Serial.println("Wifi state: WL_CONNECTED (3): The device is connected to a Wi-Fi network.");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("Wifi state: WL_CONNECT_FAILED (4): The connection attempt failed.");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("Wifi state: WL_CONNECTION_LOST (5): The connection was lost.");
        break;
      case WL_DISCONNECTED:
        Serial.println("Wifi state: WL_DISCONNECTED (6): The device is disconnected from the network.");
        break;
      default:
        Serial.println("Wifi state: Unknown status.");
        break;
    }
    Serial.println("================================");
  }
}
// Function Definitions
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;  // Already connected, no need to reconnect
  }

  Serial.println("Connecting to WiFi...");
  WiFi.hostname(currentConfig.deviceName);
  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retries++;
    if (retries > 60) {  // Timeout after 10 seconds
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }

  Serial.println("Connected!");
}

const char* getClockModeString(ClockMode mode) {
  switch (mode) {
    case CLOCK_MODE: return "Clock Mode";
    case STOPWATCH_MODE: return "Stopwatch Mode";
    case RAINBOW_MODE: return "Rainbow Mode";
    case LOVE_MODE: return "Love Mode";
    case FOOD_MODE: return "Food Mode";
    default: return "Unknown Mode";
  }
}
