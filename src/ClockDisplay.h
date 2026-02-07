#ifndef CLOCK_DISPLAY_H
#define CLOCK_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

void initClockDisplay();
void updateClockDisplay();
void updateModeDisplay();
void displayDigit(int startIndex, int digit, uint32_t color); // This line now works
void displayLetter(int startIndex, char letter, uint32_t color);

uint32_t Wheel(byte WheelPos);
void displayTime();
void displayRainbowMode();  // Ensure this is declared
void displayLoveMode();     // Ensure this is declared
void displayFoodMode();     // Ensure this is declared
void stopwatchStart();
void stopwatchStop();
void stopwatchReset();
void stopwatchAddMinute();
bool stopwatchIsRunning();
unsigned long stopwatchGetRemainingMs();

void clearstrip();


void updateBrightness();
extern Adafruit_NeoPixel strip;
extern uint8_t currentBrightness;


int getHours();
int getMinutes();



#endif // CLOCK_DISPLAY_H
