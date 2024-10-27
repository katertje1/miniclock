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

extern time_t storedSunriseTime;
extern time_t storedSunsetTime;
time_t calculateSunriseSunset(bool calculateSunrise);
SunriseSunsetTimes getSunriseSunsetTimes();

#endif // SUNRISE_SUNSET_H
