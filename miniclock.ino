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
const long debugInterval = 10000; // Interval for debug messages

int lastCalculatedDay = -1;



//#define TIMEZONE_OFFSET 2  // Central European Summer Time (CEST) is UTC+2

void setup() {
    Serial.begin(115200);
    Serial.println("Setup started");

    currentConfig = clockConfigs[selectedClock];
    Serial.print("Using device: ");
    Serial.println(currentConfig.deviceName);
    dayTimeBrightnessOffset = currentConfig.brightnessOffsetDay;
    nightTimeBrightnessOffset = currentConfig.brightnessOffsetNight;
    hourColor = currentConfig.DEFAULT_HOUR_COLOR;
    minuteColor = currentConfig.DEFAULT_MINUTE_COLOR;
    colonColor = currentConfig.DEFAULT_COLON_COLOR;    


    initClockDisplay();
    connectToWiFi();

    // Check if DST is in effect and set the appropriate offset
    if (isDST()) {
        timeClient.setTimeOffset(3600 * 2); // Set to UTC+2 for DST
    } else {
        timeClient.setTimeOffset(3600 * 1); // Set to UTC+1 for standard time
    }

    timeClient.begin();
    timeClient.update();
    initWebServer();

    Serial.println("Setup completed");
}

void loop() {
    time_t now = timeClient.getEpochTime();
    struct tm* nowInfo = localtime(&now); // Local time adjusted with timezone and DST

if (getClockModeString(currentMode)=="Clock Mode"){
    updateClockDisplay();
}
    handleWebRequests();

    if (nowInfo->tm_mday != lastCalculatedDay) {
        Serial.println("Day has changed. Recalculating sunrise and sunset times...");
        calculateSunriseSunset(true);
        calculateSunriseSunset(false);
        lastCalculatedDay = nowInfo->tm_mday;

        Serial.print("Recalculated SunriseSunset.cpp: Sunrise time: ");
        Serial.println(ctime(&storedSunriseTime)); // Sunrise time after adjustment
        Serial.print("Recalculated SunriseSunset.cpp: Sunset time: ");
        Serial.println(ctime(&storedSunsetTime)); // Sunset time after adjustment
    }

    unsigned long currentMillis = millis();
    if (currentMillis - debugPreviousMillis >= debugInterval) {
        debugPreviousMillis = currentMillis;

        Serial.println("Device :              " + String(currentConfig.deviceName));
        Serial.println("IP :                  " + WiFi.localIP().toString());

        char formattedTime[20];
        strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", nowInfo);

        Serial.print("Current Time: ");
        Serial.println(formattedTime); // Displays the current time in local format

        Serial.print("Current clock mode is: ");
        Serial.println(getClockModeString(currentMode));

        bool isDaytime = isDayTime(); // Determine if it's daytime or nighttime
        Serial.print("Current time of day is: ");
        Serial.println(isDaytime ? "Daytime" : "Nighttime");

        // Display sunrise and sunset times after timezone and DST adjustments
        Serial.print("Sunrise time: ");
        Serial.print(ctime(&storedSunriseTime)); // Local sunrise time
        Serial.print("Sunset time: ");
        Serial.print(ctime(&storedSunsetTime)); // Local sunset time

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
        if (retries > 20) {  // Timeout after 10 seconds
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