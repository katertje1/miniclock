#include "WebServer.h"
#include "Settings.h"
#include "ClockDisplay.h"
#include <ArduinoJson.h>

ESP8266WebServer server(80);

// Variables for stopwatch functionality
bool stopwatchRunning = false;
unsigned long stopwatchStart = 0;

// Function to add CORS headers globally
void addGlobalCORSHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// Handle OPTIONS (preflight) requests globally
void handleCORSPreflight() {
  addGlobalCORSHeaders();
  server.send(204);  // No content for OPTIONS preflight request
}

// Handle any request (global CORS handling)
void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    handleCORSPreflight();  // Handle OPTIONS requests
  } else {
    addGlobalCORSHeaders();  // Add CORS headers for other requests
    server.send(404, "text/plain", "Not Found");
  }
}

void handleRoot() {
  addGlobalCORSHeaders();  // Add CORS headers to all responses

  String htmlPage = "<html><head>";
  htmlPage += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";  // Add viewport meta tag for mobile responsiveness
  htmlPage += "<script src='https://code.jquery.com/jquery-3.7.1.min.js'></script><style>";
  htmlPage += "body { font-family: Arial; margin: 0; padding: 10px; }";
  htmlPage += "h1, h2, p { text-align: center; }";  // Center align text
  htmlPage += "input[type='color'] { width: 100%; max-width: 400px; margin: 5px 0; padding: 0; font-size: 16px; }"; // Remove padding from color input
  htmlPage += "button { width: 100%; max-width: 400px; padding: 15px; margin: 10px 0; font-size: 18px; cursor: pointer; }";
  htmlPage += "button:hover { background-color: #ddd; }";
  htmlPage += ".tab button.active { background-color: #ccc; }";
  htmlPage += ".tabcontent { display: none; padding: 6px 12px; border-top: none; }";

  // Notification styling for popup at the bottom
  htmlPage += "#notification {";
  htmlPage += "  visibility: hidden; position: fixed; left: 50%; bottom: 20px; transform: translateX(-50%);";
  htmlPage += "  background-color: #333; color: #fff; padding: 15px; border-radius: 5px; z-index: 1000;";
  htmlPage += "  box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.5);";
  htmlPage += "  transition: visibility 0s, opacity 0.5s ease-in-out;";
  htmlPage += "}";
  htmlPage += "#notification.show { visibility: visible; opacity: 1; }";
  // Media query to adjust layout for smaller screens
  htmlPage += "@media (max-width: 600px) {";
  htmlPage += "  h1, h2 { font-size: 24px; }";
  htmlPage += "  button { font-size: 16px; padding: 12px; }";
  htmlPage += "}";
  htmlPage += "</style></head><body>";

  // Add devicename and softwareVersion from settings.cpp
  extern const char* devicename;  // Assuming these are defined in settings.cpp
  //extern const char* softwareVersion;

  htmlPage += "<h2>" + String(currentConfig.deviceName) + "</h2>";                      // Display device name
  htmlPage += "<small>Version: " + String(softwareVersion) + "</small>";  // Display software version in subscript

  // Get the current time
  time_t currentEpochTime = timeClient.getEpochTime();  // Get current epoch time
  tm* timeInfo = localtime(&currentEpochTime);          // Convert to local time struct

  // Format the time as dd-mm-yyyy hh:mm:ss
  char timeStr[20];
  sprintf(timeStr, "%02d-%02d-%04d %02d:%02d:%02d",
          timeInfo->tm_mday,         // Day (dd)
          timeInfo->tm_mon + 1,      // Month (mm, months are 0-based in tm)
          timeInfo->tm_year + 1900,  // Year (yyyy)
          timeInfo->tm_hour,         // Hour (hh)
          timeInfo->tm_min,          // Minutes (mm)
          timeInfo->tm_sec);         // Seconds (ss)

  // Add the current date and time to the HTML
  htmlPage += "<p id='currentDateTime'>" + String(timeStr) + "</p>";  // Display formatted time

  htmlPage += "<div class='tab'>";
  htmlPage += "<button class='tablinks' onclick='openTab(event, \"Clock\")' id='clockTab'>Clock</button>";
  htmlPage += "<button class='tablinks' onclick='openTab(event, \"Stopwatch\")' id='stopwatchTab'>Stopwatch</button>";
  htmlPage += "<button class='tablinks' onclick='openTab(event, \"Love\")' id='loveTab'>Love</button>";
  htmlPage += "<button class='tablinks' onclick='openTab(event, \"Rainbow\")' id='rainbowTab'>Rainbow</button>";
  htmlPage += "<button class='tablinks' onclick='openTab(event, \"Food\")' id='foodTab'>Food</button>";
  htmlPage += "</div>";

  htmlPage += "<hr />";

  // Clock Mode Tab
  htmlPage += "<div id='Clock' class='tabcontent'>";
  htmlPage += "<button type='button' class='modeButton' data-url='/setClockMode'>Activate Clock Mode</button>";

  // Hour and Minute color pickers (initial values will be set using the fetched data)
  char ColorStr[8];  // Buffer for the color string in #RRGGBB format
  sprintf(ColorStr, "#%06X", hourColor & 0xFFFFFF);
  htmlPage += "<p>Hour Color: <input type='color' id='hourColor' name='hourColor' value='" + String(ColorStr) + "'></p>";
  sprintf(ColorStr, "#%06X", colonColor & 0xFFFFFF);
  htmlPage += "<p>Colon Color: <input type='color' id='colonColor' name='colonColor' value='" + String(ColorStr) + "'></p>"; 
  sprintf(ColorStr, "#%06X", minuteColor & 0xFFFFFF);
  htmlPage += "<p>Minute Color: <input type='color' id='minuteColor' name='minuteColor' value='" + String(ColorStr) + "'></p>";
  htmlPage += "<button type='button' onclick='resetClockColors()'>Set colors to Defaults</button>";

  // Current Brightness from API
  htmlPage += "<p>Current Brightness: <span id='currentBrightness'>"+String(currentBrightness)+"</span></p>";

  // Day and Night brightness offsets with sliders (-255 to 255)
  htmlPage += "<p>Daytime Offset: <input type='range' id='dayOffset' name='dayOffset' min='-255' max='255' value='" + String(dayTimeBrightnessOffset) + "' oninput='updateDayOffsetValue()'><span id='dayOffsetValue'>" + String(dayTimeBrightnessOffset) + "</span></p>";
  htmlPage += "<p>Nighttime Offset: <input type='range' id='nightOffset' name='nightOffset' min='-255' max='255' value='" + String(nightTimeBrightnessOffset) + "' oninput='updateNightOffsetValue()'><span id='nightOffsetValue'>" + String(nightTimeBrightnessOffset) + "</span></p>";
  htmlPage += "</div>";

  // Other tabs and buttons for stopwatch, love, rainbow, and food modes
  htmlPage += "<div id='Stopwatch' class='tabcontent'>";
  htmlPage += "<button type='button' class='modeButton' data-url='/setStopwatchMode' disabled>Activate Stopwatch Mode</button>";
  htmlPage += "<p>not implemented yet</p>";
  htmlPage += "</div>";

  htmlPage += "<div id='Love' class='tabcontent'>";
  htmlPage += "<button type='button' class='modeButton' data-url='/setLoveMode'>Activate Love Mode</button>";
  htmlPage += "</div>";

  htmlPage += "<div id='Rainbow' class='tabcontent'>";
  htmlPage += "<button type='button' class='modeButton' data-url='/setRainbowMode'>Activate Rainbow Mode</button>";
  htmlPage += "</div>";

  htmlPage += "<div id='Food' class='tabcontent'>";
  htmlPage += "<button type='button' class='modeButton' data-url='/setFoodMode'>Activate Food Mode</button>";
  htmlPage += "</div>";

  // Popup Notification element
  htmlPage += "<div id='notification'></div>";

  htmlPage += "<script>";
  htmlPage += "function openTab(evt, tabName) {";
  htmlPage += "  var i, tabcontent, tablinks;";
  htmlPage += "  tabcontent = document.getElementsByClassName('tabcontent');";
  htmlPage += "  for (i = 0; i < tabcontent.length; i++) {";
  htmlPage += "    tabcontent[i].style.display = 'none';";
  htmlPage += "  }";
  htmlPage += "  tablinks = document.getElementsByClassName('tablinks');";
  htmlPage += "  for (i = 0; i < tablinks.length; i++) {";
  htmlPage += "    tablinks[i].className = tablinks[i].className.replace(' active', '');";
  htmlPage += "  }";
  htmlPage += "  document.getElementById(tabName).style.display = 'block';";
  htmlPage += "  evt.currentTarget.className += ' active';";
  htmlPage += "}";

  // Handle mode button clicks
  htmlPage += "$(document).ready(function() {";
  htmlPage += "  $('.modeButton').click(function() {";
  htmlPage += "    var url = $(this).data('url');";
  htmlPage += "    fetch(url).then(response => {";
  htmlPage += "      if (response.ok) {";
  htmlPage += "        showNotification('Mode activated successfully!');";
  htmlPage += "      } else {";
  htmlPage += "        showNotification('Failed to activate mode');";
  htmlPage += "      }";
  htmlPage += "    }).catch(error => showNotification('Error activating mode: ' + error));";
  htmlPage += "  });";
  htmlPage += "});";

  htmlPage += "function resetClockColors() {";
  htmlPage += "  fetch('/resetColors', { method: 'POST' })";  // Assuming POST request to /resetColors
  htmlPage += "    .then(response => response.json())";       // Expecting JSON response with default colors
  htmlPage += "    .then(data => {";
  htmlPage += "      document.getElementById('hourColor').value = data.defaultHourColor;";
  htmlPage += "      document.getElementById('minuteColor').value = data.defaultMinuteColor;";
  htmlPage += "      document.getElementById('colonColor').value = data.defaultColonColor;";  //todo not there YET?
  htmlPage += "      showNotification('Colors reset to defaults');";
  htmlPage += "    })";
  htmlPage += "    .catch(error => showNotification('Error resetting colors: ' + error));";
  htmlPage += "}";

  htmlPage += "";


  htmlPage += "";
  htmlPage += "function fetchCurrentBrightness() {";
  htmlPage += "  fetch('/getBrightness')";
  htmlPage += "    .then(response => response.json())";  // Parse the response as JSON
  htmlPage += "    .then(data => {";
  htmlPage += "      document.getElementById('currentBrightness').innerText = data.brightness;";  // Display the brightness value only
  htmlPage += "    })";
  htmlPage += "    .catch(error => console.error('Error fetching brightness:', error));";
  htmlPage += "}";

  // Functions to update and display the day/night offset slider values
  htmlPage += "function updateDayOffsetValue() {";
  htmlPage += "  document.getElementById('dayOffsetValue').innerText = document.getElementById('dayOffset').value;";
  htmlPage += "}";
  htmlPage += "function updateNightOffsetValue() {";
  htmlPage += "  document.getElementById('nightOffsetValue').innerText = document.getElementById('nightOffset').value;";
  htmlPage += "}";

  // Function to submit the brightness offsets to /setBrightnessOffsets
htmlPage += "function submitBrightnessOffsets() {";
htmlPage += "  var dayOffset = document.getElementById('dayOffset').value;";
htmlPage += "  var nightOffset = document.getElementById('nightOffset').value;";
htmlPage += "  var data = {";
htmlPage += "    dayOffset: parseInt(dayOffset),";  // Ensure the value is an integer
htmlPage += "    nightOffset: parseInt(nightOffset)";  // Ensure the value is an integer
htmlPage += "  };";
htmlPage += "  fetch('/setBrightnessOffsets', {";
htmlPage += "    method: 'POST',";
htmlPage += "    headers: { 'Content-Type': 'application/json' },";
htmlPage += "    body: JSON.stringify(data)";
htmlPage += "  }).then(response => {";
htmlPage += "    if (response.ok) {";
htmlPage += "      showNotification('Brightness offsets updated successfully!');";
htmlPage += "    } else {";
htmlPage += "      showNotification('Failed to update brightness offsets');";
htmlPage += "    }";
htmlPage += "  }).catch(error => {";
htmlPage += "    console.error('Error updating brightness offsets:', error);";
htmlPage += "    showNotification('Error updating brightness offsets: ' + error);";
htmlPage += "  });";
htmlPage += "}";

// Add event listeners for the daytime and nighttime sliders
htmlPage += "document.getElementById('dayOffset').addEventListener('change', function() {";
htmlPage += "  submitBrightnessOffsets();";  // Submit the new values to the server
htmlPage += "});";

htmlPage += "document.getElementById('nightOffset').addEventListener('change', function() {";
htmlPage += "  submitBrightnessOffsets();";  // Submit the new values to the server
htmlPage += "});";
  // Function to show notification popup and auto-hide after 10 seconds
  htmlPage += "function showNotification(message) {";
  htmlPage += "  var notification = document.getElementById('notification');";
  htmlPage += "  notification.innerText = message;";
  htmlPage += "  notification.classList.add('show');";
  htmlPage += "  setTimeout(function() { notification.classList.remove('show'); }, 10000);";
  htmlPage += "}";
  
  // Fetch the current time from the backend and update the display
  htmlPage += "function fetchCurrentDateTime() {";
  htmlPage += "  fetch('/getCurrentDateTime')";
  htmlPage += "    .then(response => response.text())";
  htmlPage += "    .then(datetime => {";
  htmlPage += "      document.getElementById('currentDateTime').innerText = datetime;";
  htmlPage += "    })";
  htmlPage += "    .catch(error => console.error('Error fetching date and time:', error));";
  htmlPage += "}";

  // Fetch the day and nighttime offsets and update the sliders
  htmlPage += "function fetchBrightnessOffsets() {";
  htmlPage += "  fetch('/getBrightnessOffsets')";
  htmlPage += "    .then(response => response.json())";  // Assuming the response is JSON
  htmlPage += "    .then(data => {";
  htmlPage += "      document.getElementById('dayOffset').value = data.dayOffset;";
  htmlPage += "      document.getElementById('dayOffsetValue').innerText = data.dayOffset;";
  htmlPage += "      document.getElementById('nightOffset').value = data.nightOffset;";
  htmlPage += "      document.getElementById('nightOffsetValue').innerText = data.nightOffset;";
  htmlPage += "    })";
  htmlPage += "    .catch(error => console.error('Error fetching brightness offsets:', error));";
  htmlPage += "}";

// Reverted back without the `isSelectingColor` logic

  // Interval fetch logic without disabling fetch during color selection
  htmlPage += "setInterval(function() {";
  htmlPage += "    fetchCurrentDateTime();";
  htmlPage += "    fetchColors();"; // This fetches the color updates every 2 seconds
  htmlPage += "    fetchCurrentBrightness();";
  htmlPage += "    fetchBrightnessOffsets();";
  htmlPage += "}, 5000);";  // Every 5 seconds

htmlPage += "let isSelectingColor = false;";  // Flag to pause fetching during color selection

// Function to fetch colors from the server
htmlPage += "function fetchColors() {";
htmlPage += "  if (!isSelectingColor) {";  // Only fetch colors if not selecting
htmlPage += "    fetch('/getHourColor').then(response => response.text()).then(color => {";
htmlPage += "      const cleanColor = color.trim().replace(/[^#0-9A-Fa-f]/g, '');";
htmlPage += "      document.getElementById('hourColor').value = cleanColor;";
htmlPage += "    }).catch(error => console.error('Error fetching hour color:', error));";

htmlPage += "    fetch('/getMinuteColor').then(response => response.text()).then(color => {";
htmlPage += "      const cleanColor = color.trim().replace(/[^#0-9A-Fa-f]/g, '');";
htmlPage += "      document.getElementById('minuteColor').value = cleanColor;";
htmlPage += "    }).catch(error => console.error('Error fetching minute color:', error));";

htmlPage += "    fetch('/getColonColor').then(response => response.text()).then(color => {";
htmlPage += "      const cleanColor = color.trim().replace(/[^#0-9A-Fa-f]/g, '');";
htmlPage += "      document.getElementById('colonColor').value = cleanColor;";
htmlPage += "    }).catch(error => console.error('Error fetching colon color:', error));";
htmlPage += "  }";
htmlPage += "}";

// Pause fetching when user starts selecting a color
htmlPage += "document.getElementById('hourColor').addEventListener('input', function() {";
htmlPage += "  isSelectingColor = true;";
htmlPage += "  updateHourColor();";  // Update the clock with the selected color
htmlPage += "});";

htmlPage += "document.getElementById('minuteColor').addEventListener('input', function() {";
htmlPage += "  isSelectingColor = true;";
htmlPage += "  updateMinuteColor();";  // Update the clock with the selected color
htmlPage += "});";

htmlPage += "document.getElementById('colonColor').addEventListener('input', function() {";
htmlPage += "  isSelectingColor = true;";
htmlPage += "  updateColonColor();";  // Update the clock with the selected color
htmlPage += "});";

// Resume fetching once color selection is done
htmlPage += "document.getElementById('hourColor').addEventListener('change', function() {";
htmlPage += "  isSelectingColor = false;";
htmlPage += "  setTimeout(fetchColors, 2000);";  // Resume fetching after a brief delay
htmlPage += "});";

htmlPage += "document.getElementById('minuteColor').addEventListener('change', function() {";
htmlPage += "  isSelectingColor = false;";
htmlPage += "  setTimeout(fetchColors, 2000);";  // Resume fetching after a brief delay
htmlPage += "});";

htmlPage += "document.getElementById('colonColor').addEventListener('change', function() {";
htmlPage += "  isSelectingColor = false;";
htmlPage += "  setTimeout(fetchColors, 2000);";  // Resume fetching after a brief delay
htmlPage += "});";

// Function to update hour color on the clock
htmlPage += "function updateHourColor() {";
htmlPage += "  var newColor = document.getElementById('hourColor').value;";
htmlPage += "  fetch('/setHourColor', {";
htmlPage += "    method: 'POST',";
htmlPage += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
htmlPage += "    body: 'color=' + encodeURIComponent(newColor)";
htmlPage += "  }).then(response => {";
htmlPage += "    if (response.ok) {";
htmlPage += "    } else {";
htmlPage += "      console.error('Failed to update hour color');";
htmlPage += "    }";
htmlPage += "  }).catch(error => console.error('Error updating hour color:', error));";
htmlPage += "}";

// Function to update minute color on the clock
htmlPage += "function updateMinuteColor() {";
htmlPage += "  var newColor = document.getElementById('minuteColor').value;";
htmlPage += "  fetch('/setMinuteColor', {";
htmlPage += "    method: 'POST',";
htmlPage += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
htmlPage += "    body: 'color=' + encodeURIComponent(newColor)";
htmlPage += "  }).then(response => {";
htmlPage += "    if (response.ok) {";
htmlPage += "    } else {";
htmlPage += "      console.error('Failed to update minute color');";
htmlPage += "    }";
htmlPage += "  }).catch(error => console.error('Error updating minute color:', error));";
htmlPage += "}";

// Function to update colon color on the clock
htmlPage += "function updateColonColor() {";
htmlPage += "  var newColor = document.getElementById('colonColor').value;";
htmlPage += "  fetch('/setColonColor', {";
htmlPage += "    method: 'POST',";
htmlPage += "    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },";
htmlPage += "    body: 'color=' + encodeURIComponent(newColor)";
htmlPage += "  }).then(response => {";
htmlPage += "    if (response.ok) {";
htmlPage += "    } else {";
htmlPage += "      console.error('Failed to update colon color');";
htmlPage += "    }";
htmlPage += "  }).catch(error => console.error('Error updating colon color:', error));";
htmlPage += "}";

htmlPage += "setInterval(function() {";
htmlPage += "  fetchCurrentDateTime();";
htmlPage += "  fetchColors();";
htmlPage += "  fetchCurrentBrightness();";
htmlPage += "  fetchBrightnessOffsets();";
htmlPage += "}, 5000);";  // Every 5 seconds, fetch updates only when not selecting a color


  htmlPage += "</script>";

  htmlPage += "</body></html>";

  server.send(200, "text/html", htmlPage);  server.send(200, "text/html", htmlPage);
}

void initWebServer() {
  // Handle CORS preflight and not found requests globally
  server.onNotFound(handleNotFound);

  // Define actual routes
  server.on("/", handleRoot);
  server.on("/getDeviceName", []() { addGlobalCORSHeaders(); handleGetDeviceName(); });
  server.on("/getSoftwareVersion", []() { addGlobalCORSHeaders(); handleGetSoftwareVersion(); });
  server.on("/getCurrentMode", []() { addGlobalCORSHeaders(); handleGetCurrentMode(); });
  server.on("/getSunrise", []() { addGlobalCORSHeaders(); handleGetSunrise(); });
  server.on("/getSunset", []() { addGlobalCORSHeaders(); handleGetSunset(); });

  server.on("/setClockMode", []() { addGlobalCORSHeaders(); setClockMode(); });
  server.on("/setStopwatchMode", []() { addGlobalCORSHeaders(); setStopwatchMode(); });
  server.on("/setRainbowMode", []() { addGlobalCORSHeaders(); handleRainbowMode(); });
  server.on("/setLoveMode", []() { addGlobalCORSHeaders(); handleLoveMode(); });
  server.on("/setFoodMode", []() { addGlobalCORSHeaders(); handleFoodMode(); });

  // Stopwatch control
  server.on("/startStopwatch", []() { addGlobalCORSHeaders(); startStopwatch(); });
  server.on("/stopStopwatch", []() { addGlobalCORSHeaders(); stopStopwatch(); });
  server.on("/resetStopwatch", []() { addGlobalCORSHeaders(); resetStopwatch(); });

  // Settings and other endpoints
  server.on("/resetColors", []() { addGlobalCORSHeaders(); resetColors(); });
  server.on("/setBrightnessOffsets", []() { addGlobalCORSHeaders(); setBrightnessOffsets(); });
  server.on("/getBrightness", []() { addGlobalCORSHeaders(); getBrightness(); });
  
  // Color fetching endpoints
  server.on("/getHourColor", []() { addGlobalCORSHeaders(); handleGetHourColor(); });
  server.on("/getMinuteColor", []() { addGlobalCORSHeaders(); handleGetMinuteColor(); });
  server.on("/getColonColor", []() { addGlobalCORSHeaders(); handleGetColonColor(); });

  server.on("/setHourColor", []() { addGlobalCORSHeaders(); handleSetHourColor(); });
  server.on("/setMinuteColor", []() { addGlobalCORSHeaders(); handleSetMinuteColor(); });
  server.on("/setColonColor", []() { addGlobalCORSHeaders(); handleSetColonColor(); });

  server.on("/getCurrentDateTime", []() { addGlobalCORSHeaders(); getCurrentDateTime(); });
  server.on("/getBrightnessOffsets", []() { addGlobalCORSHeaders(); getBrightnessOffsets(); });

  server.begin();
}

void handleWebRequests() {
  server.handleClient();
}

void handleGetDeviceName() {
  server.send(200, "text/plain", currentConfig.deviceName);
}
void handleGetSoftwareVersion() {
  server.send(200, "text/plain", softwareVersion);
}
void handleGetCurrentMode() {
  server.send(200, "text/plain", getClockModeString(currentMode));
}
void handleGetSunrise(){
    SunriseSunsetTimes times = getSunriseSunsetTimes();
    time_t sunriseTime = times.sunrise;
    tm* timeInfo = localtime(&sunriseTime);
    char sunriseStr[20];
    strftime(sunriseStr, sizeof(sunriseStr), "%I:%M %p", timeInfo);  // Use "%H:%M:%S" for 24-hour format
    server.send(200, "text/plain", sunriseStr);
}
void handleGetSunset(){
    SunriseSunsetTimes times = getSunriseSunsetTimes();
    time_t sunsetTime = times.sunset;
    tm* timeInfo = localtime(&sunsetTime);
    char sunsetStr[20];
    strftime(sunsetStr, sizeof(sunsetStr), "%I:%M %p", timeInfo);  // Use "%H:%M:%S" for 24-hour format
    server.send(200, "text/plain", sunsetStr);
}

// Specific route handling functions
void setClockMode() {
  currentMode = CLOCK_MODE;
  clearstrip();
  server.send(200, "text/html", "Clock mode set. <a href=\"/\">Go Back</a>");
}

void setStopwatchMode() {
  currentMode = STOPWATCH_MODE;
  stopwatchRunning = false;
  stopwatchStart = 0;
  server.send(200, "text/html", "Stopwatch mode set. <a href=\"/\">Go Back</a>");
}

void handleRainbowMode() {
  currentMode = RAINBOW_MODE;
  displayRainbowMode();
  server.send(200, "text/html", "Switched to Rainbow Mode");
}

void handleLoveMode() {
  currentMode = LOVE_MODE;
  displayLoveMode();
  server.send(200, "text/html", "Switched to Love Mode");
}

void handleFoodMode() {
  currentMode = FOOD_MODE;
  displayFoodMode();
  server.send(200, "text/html", "Switched to Food Mode");
}

void startStopwatch() {
  if (!stopwatchRunning) {
    stopwatchRunning = true;
    stopwatchStart = millis();
  }
  server.send(200, "text/html", "Stopwatch started. <a href=\"/\">Go Back</a>");
}

void stopStopwatch() {
  if (stopwatchRunning) {
    stopwatchRunning = false;
  }
  server.send(200, "text/html", "Stopwatch stopped. <a href=\"/\">Go Back</a>");
}

void resetStopwatch() {
  stopwatchStart = 0;
  server.send(200, "text/html", "Stopwatch reset. <a href=\"/\">Go Back</a>");
}

void resetColors() {
  hourColor = currentConfig.DEFAULT_HOUR_COLOR;
  minuteColor = currentConfig.DEFAULT_MINUTE_COLOR;
  colonColor = currentConfig.DEFAULT_COLON_COLOR;

  char hourColorStr[8], minuteColorStr[8], colonColorStr[8];
  sprintf(hourColorStr, "#%06X", currentConfig.DEFAULT_HOUR_COLOR & 0xFFFFFF);
  sprintf(minuteColorStr, "#%06X", currentConfig.DEFAULT_MINUTE_COLOR & 0xFFFFFF);
  sprintf(colonColorStr, "#%06X", currentConfig.DEFAULT_COLON_COLOR & 0xFFFFFF);

  String jsonResponse = "{";
  jsonResponse += "\"defaultHourColor\": \"" + String(hourColorStr) + "\",";
  jsonResponse += "\"defaultMinuteColor\": \"" + String(minuteColorStr) + "\",";
  jsonResponse += "\"defaultColonColor\": \"" + String(colonColorStr) + "\"";
  jsonResponse += "}";

  server.send(200, "application/json", jsonResponse);
}

void getBrightness() {
  DynamicJsonDocument jsonDoc(128);
  jsonDoc["brightness"] = currentBrightness;

  String response;
  serializeJson(jsonDoc, response);
  server.send(200, "application/json", response);
}

void setBrightnessOffsets() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, server.arg("plain"));

    if (doc.containsKey("dayOffset")) {
      dayTimeBrightnessOffset = doc["dayOffset"].as<int>();
    }
    if (doc.containsKey("nightOffset")) {
      nightTimeBrightnessOffset = doc["nightOffset"].as<int>();
    }
    server.send(200, "text/html", "Brightness offsets set successfully.");
  } else {
    server.send(400, "text/html", "Missing parameters.");
  }
}

void handleGetHourColor() {
  char colorStr[8];
  sprintf(colorStr, "#%06X", hourColor & 0xFFFFFF);
  server.send(200, "text/plain", String(colorStr));
}

void handleGetMinuteColor() {
  char colorStr[8];
  sprintf(colorStr, "#%06X", minuteColor & 0xFFFFFF);
  server.send(200, "text/plain", String(colorStr));
}

void handleGetColonColor() {
  char colorStr[8];
  sprintf(colorStr, "#%06X", colonColor & 0xFFFFFF);
  server.send(200, "text/plain", String(colorStr));
}

void handleSetHourColor() {
  if (server.hasArg("color")) {
    String newColor = server.arg("color");
    hourColor = strtol(newColor.substring(1).c_str(), NULL, 16);
    server.send(200, "text/plain", "Hour color updated");
  } else {
    server.send(400, "text/plain", "Color parameter missing");
  }
}

void handleSetMinuteColor() {
  if (server.hasArg("color")) {
    String newColor = server.arg("color");
    minuteColor = strtol(newColor.substring(1).c_str(), NULL, 16);
    server.send(200, "text/plain", "Minute color updated");
  } else {
    server.send(400, "text/plain", "Color parameter missing");
  }
}

void handleSetColonColor() {
  if (server.hasArg("color")) {
    String newColor = server.arg("color");
    colonColor = strtol(newColor.substring(1).c_str(), NULL, 16);
    server.send(200, "text/plain", "Colon color updated");
  } else {
    server.send(400, "text/plain", "Missing color argument");
  }
}

void getCurrentDateTime() {
  time_t currentEpochTime = timeClient.getEpochTime();
  tm* timeInfo = localtime(&currentEpochTime);
  char timeStr[20];
  sprintf(timeStr, "%02d-%02d-%04d %02d:%02d:%02d", timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  server.send(200, "text/plain", String(timeStr));
}

void getBrightnessOffsets() {
  String jsonResponse = "{\"dayOffset\": " + String(dayTimeBrightnessOffset) + ", \"nightOffset\": " + String(nightTimeBrightnessOffset) + "}";
  server.send(200, "application/json", jsonResponse);
}