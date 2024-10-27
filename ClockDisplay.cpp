#include "ClockDisplay.h"
#include <Adafruit_NeoPixel.h>
#include "Settings.h"
#include "SunriseSunset.h"
#include <ESP8266WiFi.h>  // Include this for WiFi functionality


int targetBrightness = 0;
uint8_t currentBrightness = 128; // Or whatever initial value is appropriate

const int fadeAmount = 5;  // Adjust this value for smoother or faster transitions

// Define the necessary variables
unsigned long previousMillis = 0;  // Store the last time the display was updated
const long interval = 1000;  // Update the display every 1000 ms (1 second)

Adafruit_NeoPixel strip(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Mapping for the digits (0-9) to the LED segments
const uint8_t digitPatterns[10][7] = {
  /*
  { 1, 1, 1, 1, 1, 1, 0 },  // 0
  { 1, 1, 0, 0, 0, 0, 0 },  // 1
  { 1, 0, 1, 1, 0, 1, 1 },  // 2
  { 1, 1, 1, 0, 0, 1, 1 },  // 3
  { 1, 1, 0, 0, 1, 0, 1 },  // 4
  { 0, 1, 1, 0, 1, 1, 1 },  // 5
  { 0, 1, 1, 1, 1, 1, 1 },  // 6
  { 1, 1, 0, 0, 0, 1, 0 },  // 7
  { 1, 1, 1, 1, 1, 1, 1 },  // 8
  { 1, 1, 1, 0, 1, 1, 1 }   // 9
};*/
      //1, 2, 3, 4, 5, 6, 7,
      { 1, 1, 1, 1, 0, 1, 1 },  // 0
      { 0, 0, 0, 1, 0, 1, 0 },  // 1
      { 1, 0, 1, 1, 1, 0, 1 },  // 2
      { 0, 0, 1, 1, 1, 1, 1 },  // 3
      { 0, 1, 0, 1, 1, 1, 0 },  // 4
      { 0, 1, 1, 0, 1, 1, 1 },  // 5
      { 1, 1, 1, 0, 1, 1, 1 },  // 6
      { 0, 0, 1, 1, 0, 1, 0 },  // 7
      { 1, 1, 1, 1, 1, 1, 1 },  // 8
      { 0, 1, 1, 1, 1, 1, 1 }   // 9
    };

#include <map>

// Define the letter patterns using a map
std::map<char, const uint8_t*> letterPatternsMap = {
    {'D', (const uint8_t[7]){1, 1, 0, 1, 1, 1, 0}},  // D
    {'E', (const uint8_t[7]){1, 1, 1, 0, 1, 0, 1}},  // E
    {'F', (const uint8_t[7]){1, 1, 1, 0, 1, 0, 0}},  // F
    {'L', (const uint8_t[7]){1, 1, 0, 0, 0, 0, 1}},  // L
    {'O', (const uint8_t[7]){1, 1, 1, 1, 0, 1, 1}},  // O
    {'V', (const uint8_t[7]){1, 1, 0, 1, 0, 1, 1}},  // V
    {'d', (const uint8_t[7]){1, 0, 0, 1, 1, 1, 1}},  // d
    {'o', (const uint8_t[7]){1, 0, 0, 0, 1, 1, 1}}   // o
};

// Define the pins for the digits and colon
const int digitPins[7] = {0, 1, 2, 3, 4, 5, 6};  // Adjust these according to your wiring
const int colonTopLED = 28;   // Index for the top colon LED
const int colonBottomLED = 29; // Index for the bottom colon LED

// Declare the functions before their usage
void displayDigit(int startIndex, int digit, uint32_t color);
void displayColon();
void displayLetter(int startIndex, int letterIndex, uint32_t color);
uint32_t Wheel(byte WheelPos);

void initClockDisplay() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

int getHours() {
    time_t now = timeClient.getEpochTime();
    struct tm* nowInfo = localtime(&now);
    return nowInfo->tm_hour;
}

int getMinutes() {
    time_t now = timeClient.getEpochTime();
    struct tm* nowInfo = localtime(&now);
    return nowInfo->tm_min;
}


void updateClockDisplay() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        isDaytime = isDayTime();
        updateBrightness();

        int hours = getHours();
        int minutes = getMinutes();

        // Determine whether to show the leading zero for the hour
        if (hours < 10) {
            // Clear the first digit
            for (int i = 0; i < 7; i++) {
                strip.setPixelColor(digitPins[i], 0);  // Turn off all LEDs for the first digit
            }
            displayDigit(7, hours % 10, isDaytime ? hourColor : strip.Color(255, 0, 0));  // Display the hour without the leading zero
        } else {
            displayDigit(0, hours / 10, isDaytime ? hourColor : strip.Color(255, 0, 0));  // Display the first digit of the hour
            displayDigit(7, hours % 10, isDaytime ? hourColor : strip.Color(255, 0, 0));  // Display the second digit of the hour
        }

        displayColon();
        displayDigit(16, minutes / 10, isDaytime ? minuteColor : strip.Color(255, 0, 0));
        displayDigit(23, minutes % 10, isDaytime ? minuteColor : strip.Color(255, 0, 0));

        strip.show();
    }
}

void displayDigit(int startIndex, int digit, uint32_t color) {
    for (int i = 0; i < 7; i++) {
        if (digitPatterns[digit][i]) {
            strip.setPixelColor(startIndex + digitPins[i], color);
        } else {
            strip.setPixelColor(startIndex + digitPins[i], 0);  // Turn off the segment
        }
    }
}

void displayColon() {
    strip.setPixelColor(colonTopLED, colonColor);
    strip.setPixelColor(colonBottomLED, colonColor);
}

void displayLetter(int startIndex, char letter, uint32_t color) {
    if (letterPatternsMap.find(letter) != letterPatternsMap.end()) {
        const uint8_t* pattern = letterPatternsMap[letter];
        for (int i = 0; i < 7; i++) {
            if (pattern[i]) {
                strip.setPixelColor(startIndex + i, color);
            } else {
                strip.setPixelColor(startIndex + i, 0);  // Turn off the LED
            }
        }
        strip.show();
    } else {
        // Handle case where letter is not found in the map
        Serial.println("Letter not found in pattern map");
    }
}

uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else if (WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    } else {
        WheelPos -= 170;
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
}
void updateBrightness() {
    isDaytime = isDayTime();
    int brightnessOffset = isDaytime ? dayTimeBrightnessOffset : nightTimeBrightnessOffset;

    int sensorValue = analogRead(LIGHT_SENSOR_PIN);
    
    // Invert the sensor value to ensure more light results in higher brightness
    int invertedSensorValue = 255 - (sensorValue / 4);
    targetBrightness = max(3, min(255, invertedSensorValue + brightnessOffset));  // Adjust brightness with offset

    // Only adjust brightness if the difference is greater than a small threshold
    int brightnessThreshold = 2;  // Prevents jitter
    if (abs(currentBrightness - targetBrightness) > brightnessThreshold) {
        if (currentBrightness < targetBrightness) {
            currentBrightness = min(currentBrightness + fadeAmount, targetBrightness);
        } else if (currentBrightness > targetBrightness) {
            currentBrightness = max(currentBrightness - fadeAmount, targetBrightness);
        }

        strip.setBrightness(currentBrightness);
        strip.show();
    }
}
void displayRainbowMode() {
  strip.clear();
    Serial.println("displayRainbowMode called");
    for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
        for(int i=0; i<strip.numPixels(); i++) {
            int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
        }
        strip.show();
        delay(20);
    }
}

void displayLoveMode() {
    strip.clear();
    Serial.println("displayLoveMode called");
    displayLetter(0, 'L', strip.Color(255, 0, 0)); // L
    displayLetter(7, 'O', strip.Color(255, 0, 0)); // O
    displayLetter(16, 'V', strip.Color(255, 0, 0)); // V
    displayLetter(23, 'E', strip.Color(255, 0, 0)); // E
    strip.show();
}

void displayFoodMode() {
  strip.clear();
    Serial.println("displayFoodMode called");
    for (int i = 0; i < 10; i++) {
        displayLetter(0, 'F', strip.Color(255, 0, 0)); // F
        displayLetter(7, 'o', strip.Color(255, 0, 0)); // o
        displayLetter(16, 'o', strip.Color(255, 0, 0)); // o
        displayLetter(23, 'd', strip.Color(255, 0, 0)); // d
        strip.show();
        delay(500);
        strip.clear();
        strip.show();
        delay(500);
        displayLetter(0, 'F', strip.Color(5, 213, 255)); // F
        displayLetter(7, 'o', strip.Color(5, 213, 255)); // o
        displayLetter(16, 'o', strip.Color(5, 213, 255)); // o
        displayLetter(23, 'd', strip.Color(5, 213, 255)); // d
        strip.show();
        delay(500);
        strip.clear();
        strip.show();
        delay(500);        
    }
    currentMode = CLOCK_MODE;
    clearstrip();
}
void clearstrip(){
  strip.clear();
}