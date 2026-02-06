#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

enum ClockMode { CLOCK_MODE, STOPWATCH_MODE, RAINBOW_MODE, LOVE_MODE, FOOD_MODE };  // available modes for the clock
extern ClockMode currentMode;                       //current mode of the clock
extern const char* getClockModeString(ClockMode mode);


extern time_t lastSunriseTime;
extern time_t lastSunsetTime;

// Declare selectedClock as an index (int)
struct ClockSettings {
    const char* deviceName;
    uint32_t DEFAULT_HOUR_COLOR;
    uint32_t DEFAULT_MINUTE_COLOR;
    uint32_t DEFAULT_COLON_COLOR;
    uint32_t NIGHT_HOUR_COLOR;
    uint32_t NIGHT_MINUTE_COLOR;
    uint32_t NIGHT_COLON_COLOR;
    int dayTimeBrightnessOffset;
    int nightTimeBrightnessOffset;
};

extern int selectedClock;
extern ClockSettings clockConfigs[];
extern ClockSettings currentConfig;

//wifi
extern const char* ssid;                            //wifi networkname
extern const char* password;                        //wifi password
extern const char* ntpServerName;                   //time server the program uses

//time
extern WiFiUDP ntpUDP;                              //connection for time server
extern NTPClient timeClient;                        //keeper of time
extern const char* ntpServerName;                   //url of the timeserver

//default clock colors
extern uint32_t DEFAULT_HOUR_COLOR;
extern uint32_t DEFAULT_MINUTE_COLOR;
extern uint32_t DEFAULT_COLON_COLOR;

extern uint32_t NIGHT_HOUR_COLOR;
extern uint32_t NIGHT_MINUTE_COLOR;
extern uint32_t NIGHT_COLON_COLOR;

//brightness
extern int dayTimeBrightnessOffset;                 //offset value for the brightness during daytime
extern int nightTimeBrightnessOffset;               //offset value for the brightness during daytime

//clock location
extern const double LATITUDE;                       //latitude of the location of the clock
extern const double LONGITUDE;                      //longitude of the location of the clock

extern bool isDaytime;                              //is it currently day? (or night)
extern unsigned long modeStartTime;
extern uint8_t currentBrightness;                   //current Brightness value
extern const char* softwareVersion;                 //software version this clock is running

extern uint32_t hourColor;
extern uint32_t minuteColor;
extern uint32_t colonColor;

#define TIMEZONE_OFFSET 1  // Adjust this to your specific timezone offset (e.g., 1 for UTC+1)

//////////////////////////
//Variables that should not be changed (unless you know what you are doing)
//////////////////////////

// Pin and device configuration
#define LED_PIN D5
#define NUM_PIXELS 32
#define LIGHT_SENSOR_PIN A0

#endif // SETTINGS_H