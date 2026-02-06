/*
#include "Settings.h"


ClockSettings clockConfigs[] = {
    { "WifiClock",    "Wifi name", "Wifi password", 0x00a2ff, 0x00a2ff, 0x00a2ff, 0xFF0000, 0xFF0000, 0xFF0000, 50, -150 }
};
int selectedClock = 0;
ClockSettings currentConfig;


// location to calculate sunrise/sunset
const double LATITUDE = 52.3676; // Latitude for Amsterdam
const double LONGITUDE = 4.9041; // Longitude for Amsterdam
const double ZENITH = 90.833; // Standard Zenith for sunrise/sunset

//timeserver
WiFiUDP ntpUDP;
const char* ntpServerName = "pool.ntp.org";
NTPClient timeClient(ntpUDP, ntpServerName, 0);


/////////////////////////
// no need to change
/////////////////////////

// default colors for the clock
uint32_t hourColor = currentConfig.DEFAULT_HOUR_COLOR;
uint32_t minuteColor = currentConfig.DEFAULT_MINUTE_COLOR;
uint32_t colonColor = currentConfig.DEFAULT_COLON_COLOR;

//brightness offset
int dayTimeBrightnessOffset = currentConfig.dayTimeBrightnessOffset;
int nightTimeBrightnessOffset = currentConfig.nightTimeBrightnessOffset;


// Define the actual variables
ClockMode currentMode = ClockMode::CLOCK_MODE;    // Initialize with a valid ClockMode value
unsigned long modeStartTime = 0;
bool isDaytime = true;
const char* softwareVersion = "0.210";

*/
