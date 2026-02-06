#include "SunriseSunset.h"
#include "Settings.h"
#include <math.h>
#include <time.h>

// Constants
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

// Declare global variables to store sunrise and sunset times
time_t storedSunriseTime = 0;
time_t storedSunsetTime = 0;

#define TIMEZONE_OFFSET 1  // Set this according to your local timezone (e.g., UTC+2 for CEST)



// Days in each month for non-leap years and leap years
const int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const int days_in_month_leap[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// Function to check if a year is a leap year
bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Function to manually calculate time_t from tm structure in UTC
time_t manualTimeT(struct tm* timeinfo) {
    int year = timeinfo->tm_year + 1900;
    int month = timeinfo->tm_mon;
    int day = timeinfo->tm_mday - 1;  // Convert day to zero-indexed
    int hour = timeinfo->tm_hour;
    int minute = timeinfo->tm_min;
    int second = timeinfo->tm_sec;

    // Calculate days since epoch (1970-01-01)
    time_t days = (year - 1970) * 365 + (year - 1969) / 4 - (year - 1901) / 100 + (year - 1601) / 400;
    const int* days_per_month = isLeapYear(year) ? days_in_month_leap : days_in_month;

    for (int i = 0; i < month; ++i) {
        days += days_per_month[i];
    }

    days += day;

    // Convert days to seconds
    time_t seconds = days * 24 * 3600 + hour * 3600 + minute * 60 + second;

    return seconds;
}

void calculateSunriseSunset(bool isSunrise, int year, int month, int day) {
    struct tm timeinfo = {0};

    // Latitude and longitude from Settings.h
    float lat = LATITUDE;
    float lng = LONGITUDE;

    // Calculate the day of the year
    int N1 = floor(275 * month / 9);
    int N2 = floor((month + 9) / 12);
    int N3 = (1 + floor((year - 4 * floor(year / 4) + 2) / 3));
    int N = N1 - (N2 * N3) + day - 30;

    // Convert the longitude to hour value and calculate an approximate time
    float lngHour = lng / 15.0;
    float t = isSunrise ? N + ((6 - lngHour) / 24) : N + ((18 - lngHour) / 24);

    // Calculate the Sun's mean anomaly
    float M = (0.9856 * t) - 3.289;

    // Sun's true longitude
    float L = fmod(M + (1.916 * sin(DEG_TO_RAD * M)) + (0.020 * sin(2 * DEG_TO_RAD * M)) + 282.634, 360.0);

    // Sun's right ascension
    float RA = RAD_TO_DEG * atan(0.91764 * tan(DEG_TO_RAD * L));
    RA = fmod(RA + 360.0, 360.0);
    float Lquadrant = floor(L / 90.0) * 90.0;
    float RAquadrant = floor(RA / 90.0) * 90.0;
    RA = RA + (Lquadrant - RAquadrant);
    RA /= 15.0;

    // Sun's declination
    float sinDec = 0.39782 * sin(DEG_TO_RAD * L);
    float cosDec = cos(asin(sinDec));

    float cosH = (cos(DEG_TO_RAD * 90.833) - (sinDec * sin(DEG_TO_RAD * lat))) / (cosDec * cos(DEG_TO_RAD * lat));
    if (cosH > 1 || cosH < -1) return;

    float H = isSunrise ? 360.0 - RAD_TO_DEG * acos(cosH) : RAD_TO_DEG * acos(cosH);
    H /= 15.0;

    float T = H + RA - (0.06571 * t) - 6.622;
    if (T < 0) T += 24.0;
    T = fmod(T + 24.0, 24.0);  // Normalize

    float UT = fmod(T - lngHour + 24.0, 24.0);
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = int(UT);
    timeinfo.tm_min = int((UT - int(UT)) * 60);
    timeinfo.tm_sec = 0;

    time_t finalTime = manualTimeT(&timeinfo);

    Serial.print(isSunrise ? "Sunrise UTC Time before adjustment: " : "Sunset UTC Time before adjustment: ");
    Serial.println(ctime(&finalTime));

    int timezoneOffset = TIMEZONE_OFFSET;
    finalTime += timezoneOffset * 3600;

    Serial.print(isSunrise ? "Sunrise Local Time after adjustment: " : "Sunset Local Time after adjustment: ");
    Serial.println(ctime(&finalTime));

    if (isSunrise) {
        storedSunriseTime = finalTime;
    } else {
        storedSunsetTime = finalTime;
    }
}

SunriseSunsetTimes getSunriseSunsetTimes() {
    // This function will return the stored times without recalculating
    SunriseSunsetTimes times;
times.sunrise = storedSunriseTime;
times.sunset = storedSunsetTime;
    return times;
}

// Function to determine if it is currently daytime
bool isDayTime() {
    SunriseSunsetTimes times = getSunriseSunsetTimes();
    time_t now = timeClient.getEpochTime();
   return now >= times.sunrise && now < times.sunset;
}

bool isDST() {
    time_t now = timeClient.getEpochTime();
    struct tm *timeinfo = gmtime(&now);

    int y = timeinfo->tm_year + 1900;
    int m = timeinfo->tm_mon + 1;
    int d = timeinfo->tm_mday;

    // Calculate last Sunday of March
    int lastMarchSunday = 31 - ((5 + y * 5 / 4) % 7);
    // Calculate last Sunday of October
    int lastOctoberSunday = 31 - ((2 + y * 5 / 4) % 7);

    if (m < 3 || m > 10) return false;
    if (m > 3 && m < 10) return true;
    if (m == 3 && d >= lastMarchSunday) return true;
    if (m == 10 && d < lastOctoberSunday) return true;
    return false;
}
int getDSTOffset() {
  time_t now = timeClient.getEpochTime();
  struct tm *timeinfo = gmtime(&now);
  if (!timeinfo) return 3600;  // fallback if something fails

  int month = timeinfo->tm_mon + 1;
  int day = timeinfo->tm_mday;
  int hour = timeinfo->tm_hour;

  // DST from last Sunday in March (from 2:00) to last Sunday in October (until 3:00)
  if (month > 3 && month < 10) return 7200;
  if (month < 3 || month > 10) return 3600;

  // Last Sunday of March
  if (month == 3) {
    int lastSunday = 31 - ((timeinfo->tm_wday + 31 - day) % 7);
    if (day > lastSunday || (day == lastSunday && hour >= 2)) return 7200;
    return 3600;
  }

  // Last Sunday of October
  if (month == 10) {
    int lastSunday = 31 - ((timeinfo->tm_wday + 31 - day) % 7);
    if (day < lastSunday || (day == lastSunday && hour < 3)) return 7200;
    return 3600;
  }

  return 3600;
}
