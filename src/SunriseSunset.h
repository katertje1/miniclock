#ifndef SUNRISE_SUNSET_H
#define SUNRISE_SUNSET_H

#include <time.h>  // Ensure time_t is recognized

struct SunriseSunsetTimes {
    time_t sunrise;
    time_t sunset;
};

extern SunriseSunsetTimes getSunriseSunsetTimes();
bool isDayTime();
bool isDST();
int getDSTOffset();


extern time_t storedSunriseTime;
extern time_t storedSunsetTime;
void calculateSunriseSunset(bool isSunrise, int year, int month, int day);
SunriseSunsetTimes getSunriseSunsetTimes();
#endif // SUNRISE_SUNSET_H
