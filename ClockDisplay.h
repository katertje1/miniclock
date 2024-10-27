#ifndef CLOCK_DISPLAY_H
#define CLOCK_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

void initClockDisplay();
void updateClockDisplay();
void displayDigit(int startIndex, int digit, uint32_t color); // This line now works
void displayLetter(int startIndex, int letterIndex, uint32_t color);

uint32_t Wheel(); // This line also works now
void displayTime();
void displayRainbowMode();  // Ensure this is declared
void displayLoveMode();     // Ensure this is declared
void displayFoodMode();     // Ensure this is declared

void clearstrip();


void updateBrightness();
extern Adafruit_NeoPixel strip;
extern uint8_t currentBrightness;


int getHours();
int getMinutes();



#endif // CLOCK_DISPLAY_H
