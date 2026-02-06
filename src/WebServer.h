#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESP8266WebServer.h>
#include "Settings.h" 
#include "SunriseSunset.h"

extern ESP8266WebServer server;

void initWebServer();
void handleRoot();
void handleGetDeviceName();
void handleGetSoftwareVersion();
void handleGetCurrentMode();
void handleGetSunrise();
void handleGetSunset();

void setClockMode();
void setStopwatchMode();
void handleRainbowMode();
void startStopwatch();
void stopStopwatch();
void resetStopwatch();
void handleLoveMode();
void handleFoodMode();
void setBrightnessOffsets();
void handleWebRequests();
void resetColors();
void getBrightness();
void handleGetHourColor();
void handleGetMinuteColor();
void handleGetColonColor();
void handleSetHourColor();
void handleSetMinuteColor();
void getCurrentDateTime();
void getBrightnessOffsets();
void handleSetColonColor();


String getModeName(ClockMode mode);  // Properly declare getModeName with ClockMode

#endif // WEBSERVER_H
