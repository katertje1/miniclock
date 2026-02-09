#include "WebServer.h"
#include "Settings.h"
#include "ClockDisplay.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>

ESP8266WebServer server(80);
static bool fsReady = false;
static uint32_t diagnosticsRequestCount = 0;
// Keep large scratch buffers out of the limited cont stack on ESP8266.
static char diagResponse[2048];
static char diagIpStr[16];
static char diagSubnetStr[16];
static char diagGatewayStr[16];
static char diagDnsStr[16];
static char diagMacStr[18];
static char diagNowTs[24];
static char diagResetTs[24];
static char diagResetReason[192];
static char diagResetInfo[448];
static char diagSsid[80];
static bool diagResetCached = false;
static unsigned long diagLastBuildMs = 0;
static const unsigned long DIAG_BUILD_INTERVAL_MS = 2000;
static char statusResponse[896];
static char statusTimeStr[24];
static char statusSunriseStr[16];
static char statusSunsetStr[16];
static char statusHourColorStr[8];
static char statusMinuteColorStr[8];
static char smallJsonResponse[160];
static char clockListResponse[768];
static char genericTimeStr[24];
static char diagSummaryResponse[512];
static char diagResetInfoShort[128];

static void ensureResetInfoCached() {
  if (diagResetCached) return;
  String rr = ESP.getResetReason();
  String ri = ESP.getResetInfo();
  rr.toCharArray(diagResetReason, sizeof(diagResetReason));
  ri.toCharArray(diagResetInfo, sizeof(diagResetInfo));
  diagResetCached = true;
}

static const char* wifiStatusToString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "WL_UNKNOWN";
  }
}

static void formatTime12h(time_t epoch, char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  tm* info = localtime(&epoch);
  if (!info) {
    snprintf(out, outLen, "--:-- --");
    return;
  }
  int hour24 = info->tm_hour;
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;
  const char* ampm = (hour24 < 12) ? "AM" : "PM";
  snprintf(out, outLen, "%02d:%02d %s", hour12, info->tm_min, ampm);
}

static void formatLocalDateTime(time_t epoch, char* out, size_t outLen) {
  if (!out || outLen == 0) return;
  tm* info = localtime(&epoch);
  if (!info) {
    snprintf(out, outLen, "0000-00-00 00:00:00");
    return;
  }
  snprintf(out, outLen, "%04d-%02d-%02d %02d:%02d:%02d",
           info->tm_year + 1900, info->tm_mon + 1, info->tm_mday,
           info->tm_hour, info->tm_min, info->tm_sec);
}

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

void handleController() {
  if (!fsReady) {
    fsReady = LittleFS.begin();
  }
  if (!fsReady) {
    server.send(500, "text/plain", "Filesystem not available");
    return;
  }
  if (!LittleFS.exists("/controller.html")) {
    server.send(404, "text/plain", "controller.html not found");
    return;
  }
  File f = LittleFS.open("/controller.html", "r");
  if (!f) {
    server.send(500, "text/plain", "Failed to open controller.html");
    return;
  }
  server.streamFile(f, "text/html");
  f.close();
}

void handleGetClockList() {
  int n = snprintf(clockListResponse, sizeof(clockListResponse), "[");
  if (n < 0 || n >= (int)sizeof(clockListResponse)) {
    server.send(500, "text/plain", "Clock list too large");
    return;
  }
  int used = n;
  for (int i = 0; i < clockConfigCount; i++) {
    n = snprintf(clockListResponse + used, sizeof(clockListResponse) - used,
                 "%s\"%s\"",
                 (i == 0) ? "" : ",",
                 clockConfigs[i].deviceName);
    if (n < 0 || n >= (int)(sizeof(clockListResponse) - used)) {
      server.send(500, "text/plain", "Clock list too large");
      return;
    }
    used += n;
  }
  n = snprintf(clockListResponse + used, sizeof(clockListResponse) - used, "]");
  if (n < 0 || n >= (int)(sizeof(clockListResponse) - used)) {
    server.send(500, "text/plain", "Clock list too large");
    return;
  }
  server.send(200, "application/json", clockListResponse);
}

void handleRoot() {
  addGlobalCORSHeaders();  // Add CORS headers to all responses

  // Serve root page from LittleFS to avoid building a very large String on heap.
  if (!fsReady) {
    fsReady = LittleFS.begin();
  }
  if (!fsReady) {
    server.send(500, "text/plain", "Filesystem not available");
    return;
  }
  if (!LittleFS.exists("/index.html")) {
    server.send(404, "text/plain", "index.html not found");
    return;
  }
  File rootFile = LittleFS.open("/index.html", "r");
  if (!rootFile) {
    server.send(500, "text/plain", "Failed to open index.html");
    return;
  }
  server.streamFile(rootFile, "text/html");
  rootFile.close();
  return;

  String htmlPage;
  // Reserve once to reduce heap fragmentation from many concatenations below.
  htmlPage.reserve(16384);
  htmlPage = "<html><head>";
  htmlPage += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";  // Add viewport meta tag for mobile responsiveness
  htmlPage += "<script src='https://code.jquery.com/jquery-3.7.1.min.js'></script><style>";
  htmlPage += "body { font-family: Arial; margin: 0; padding: 10px; }";
  htmlPage += "h1, h2, p { text-align: center; }";  // Center align text
  htmlPage += "input[type='color'] { width: 100%; max-width: 400px; margin: 5px 0; padding: 0; font-size: 16px; }"; // Remove padding from color input
  htmlPage += "button { width: 100%; max-width: 400px; padding: 15px; margin: 10px 0; font-size: 18px; cursor: pointer; }";
  htmlPage += "button:hover { background-color: #ddd; }";
  htmlPage += ".tab button.active { background-color: #ccc; }";
  htmlPage += ".tabcontent { display: none; padding: 6px 12px; border-top: none; }";
  htmlPage += "#diagBadge { display: inline-block; margin-top: 10px; padding: 8px 12px; border-radius: 8px; font-weight: bold; cursor: pointer; }";
  htmlPage += ".diagGood { background: #c8e6c9; color: #1b5e20; }";
  htmlPage += ".diagBad { background: #ffcdd2; color: #b71c1c; }";
  htmlPage += ".diagUnknown { background: #eceff1; color: #37474f; }";
  htmlPage += "#diagModal { display: none; position: fixed; z-index: 1100; left: 0; top: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.45); }";
  htmlPage += "#diagModalContent { background: #fff; margin: 8% auto; padding: 14px; border-radius: 8px; width: 92%; max-width: 760px; }";
  htmlPage += "#diagText { width: 100%; height: 220px; font-family: monospace; font-size: 13px; }";
  htmlPage += ".diagBtnRow { text-align: right; margin-top: 8px; }";

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

  // Add device name and software version from settings.cpp

  htmlPage += "<h2>" + String(currentConfig.deviceName) + "</h2>";                      // Display device name
  htmlPage += "<small>Version: " + String(softwareVersion) + "</small>";  // Display software version in subscript

  // Get the current time
  time_t currentEpochTime = timeClient.getEpochTime();  // Get current epoch time
  tm* timeInfo = localtime(&currentEpochTime);          // Convert to local time struct

  // Format the time as dd-mm-yyyy hh:mm:ss
  char timeStr[64];
  snprintf(timeStr, sizeof(timeStr), "%02d-%02d-%04d %02d:%02d:%02d",
          timeInfo->tm_mday,         // Day (dd)
          timeInfo->tm_mon + 1,      // Month (mm, months are 0-based in tm)
          timeInfo->tm_year + 1900,  // Year (yyyy)
          timeInfo->tm_hour,         // Hour (hh)
          timeInfo->tm_min,          // Minutes (mm)
          timeInfo->tm_sec);         // Seconds (ss)

  // Add the current date and time to the HTML
  htmlPage += "<p id='currentDateTime'>" + String(timeStr) + "</p>";  // Display formatted time
  htmlPage += "<p><span id='diagBadge' class='diagUnknown' onclick='openDiagnosticsModal()'>Diagnostics: Unknown</span></p>";

  htmlPage += "<div class='tab'>";
  htmlPage += "<button class='tablinks' onclick='openTab(event, \"Clock\")' id='clockTab'>Clock</button>";
  htmlPage += "<button class='tablinks' onclick='openTab(event, \"Stopwatch\")' id='stopwatchTab'>Stopwatch</button>";
  htmlPage += "<button class='tablinks' onclick='openTab(event, \"RainbowClock\")' id='rainbowClockTab'>Rainbow Clock</button>";
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
  htmlPage += "<button type='button' class='modeButton' data-url='/setStopwatchMode'>Activate Stopwatch Mode</button>";
  htmlPage += "<button type='button' onclick='stopwatchAction(\"start\")'>Start</button>";
  htmlPage += "<button type='button' onclick='stopwatchAction(\"stop\")'>Stop</button>";
  htmlPage += "<button type='button' onclick='stopwatchAction(\"reset\")'>Reset</button>";
  htmlPage += "<button type='button' onclick='stopwatchAction(\"addMinute\")'>Add 1 minute</button>";
  htmlPage += "<p>Stopwatch: <span id='stopwatchDisplay'>00:00</span> <small id='stopwatchState'>(stopped)</small></p>";
  htmlPage += "</div>";

  htmlPage += "<div id='RainbowClock' class='tabcontent'>";
  htmlPage += "<button type='button' class='modeButton' data-url='/setRainbowClockMode'>Activate Rainbow Clock Mode</button>";
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
  htmlPage += "<div id='diagModal'>";
  htmlPage += "  <div id='diagModalContent'>";
  htmlPage += "    <h3>Diagnostics</h3>";
  htmlPage += "    <textarea id='diagText' readonly></textarea>";
  htmlPage += "    <div class='diagBtnRow'>";
  htmlPage += "      <button type='button' onclick='copyDiagnostics()'>Copy</button>";
  htmlPage += "      <button type='button' onclick='closeDiagnosticsModal()'>Close</button>";
  htmlPage += "    </div>";
  htmlPage += "  </div>";
  htmlPage += "</div>";

  htmlPage += "<script>";
  htmlPage += "let lastDiagnostics = null;";
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

htmlPage += "function classifyDiagnostics(diag) {";
htmlPage += "  if (!diag || !diag.resetReason) { return { text: 'Diagnostics: Unknown', cls: 'diagUnknown' }; }";
htmlPage += "  const reason = String(diag.resetReason || '');";
htmlPage += "  const info = String(diag.resetInfo || '');";
htmlPage += "  const combined = (reason + ' ' + info).toLowerCase();";
htmlPage += "  const badSignals = ['exception', 'fatal', 'wdt', 'watchdog', 'abort', 'panic', 'crash'];";
htmlPage += "  const isBad = badSignals.some(s => combined.includes(s));";
htmlPage += "  if (isBad) { return { text: 'Diagnostics: Crash', cls: 'diagBad' }; }";
htmlPage += "  if (reason.indexOf('Software/System restart') >= 0) { return { text: 'Diagnostics: Software/System restart', cls: 'diagGood' }; }";
htmlPage += "  return { text: 'Diagnostics: OK', cls: 'diagGood' };";
htmlPage += "}";

htmlPage += "function fetchDiagnostics() {";
htmlPage += "  fetch('/getDiagnostics')";
htmlPage += "    .then(response => response.json())";
htmlPage += "    .then(diag => {";
htmlPage += "      lastDiagnostics = diag;";
htmlPage += "      const badge = document.getElementById('diagBadge');";
htmlPage += "      const c = classifyDiagnostics(diag);";
htmlPage += "      badge.classList.remove('diagGood', 'diagBad', 'diagUnknown');";
htmlPage += "      badge.classList.add(c.cls);";
htmlPage += "      badge.innerText = c.text;";
htmlPage += "      badge.title = (diag.resetReason || '') + ' | ' + (diag.resetInfo || '');";
htmlPage += "    })";
htmlPage += "    .catch(() => {";
htmlPage += "      const badge = document.getElementById('diagBadge');";
htmlPage += "      badge.classList.remove('diagGood', 'diagBad');";
htmlPage += "      badge.classList.add('diagUnknown');";
htmlPage += "      badge.innerText = 'Diagnostics: Unknown';";
htmlPage += "      badge.title = 'Could not fetch /getDiagnostics';";
htmlPage += "    });";
htmlPage += "}";

htmlPage += "function openDiagnosticsModal() {";
htmlPage += "  const modal = document.getElementById('diagModal');";
htmlPage += "  const text = document.getElementById('diagText');";
htmlPage += "  if (lastDiagnostics) {";
htmlPage += "    text.value = JSON.stringify(lastDiagnostics, null, 2);";
htmlPage += "  } else {";
htmlPage += "    text.value = 'No diagnostics fetched yet.';";
htmlPage += "  }";
htmlPage += "  modal.style.display = 'block';";
htmlPage += "  text.focus();";
htmlPage += "  text.select();";
htmlPage += "}";

htmlPage += "function closeDiagnosticsModal() {";
htmlPage += "  document.getElementById('diagModal').style.display = 'none';";
htmlPage += "}";

htmlPage += "function copyDiagnostics() {";
htmlPage += "  const text = document.getElementById('diagText');";
htmlPage += "  text.focus();";
htmlPage += "  text.select();";
htmlPage += "  if (navigator.clipboard && window.isSecureContext) {";
htmlPage += "    navigator.clipboard.writeText(text.value).then(() => showNotification('Diagnostics copied'));";
htmlPage += "  } else {";
htmlPage += "    document.execCommand('copy');";
htmlPage += "    showNotification('Diagnostics copied');";
htmlPage += "  }";
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

htmlPage += "function formatStopwatch(seconds) {";
htmlPage += "  const m = Math.floor(seconds / 60);";
htmlPage += "  const s = seconds % 60;";
htmlPage += "  return String(m).padStart(2, '0') + ':' + String(s).padStart(2, '0');";
htmlPage += "}";

htmlPage += "function fetchStopwatchStatus() {";
htmlPage += "  fetch('/getStopwatchStatus')";
htmlPage += "    .then(response => response.json())";
htmlPage += "    .then(data => {";
htmlPage += "      document.getElementById('stopwatchDisplay').innerText = formatStopwatch(data.remainingSeconds || 0);";
htmlPage += "      document.getElementById('stopwatchState').innerText = data.running ? '(running)' : '(stopped)';";
htmlPage += "    })";
htmlPage += "    .catch(error => console.error('Error fetching stopwatch status:', error));";
htmlPage += "}";

htmlPage += "function stopwatchAction(action) {";
htmlPage += "  const map = { start: '/startStopwatch', stop: '/stopStopwatch', reset: '/resetStopwatch', addMinute: '/addStopwatchMinute' };";
htmlPage += "  const url = map[action];";
htmlPage += "  if (!url) return;";
htmlPage += "  fetch(url, { method: 'POST' })";
htmlPage += "    .then(response => {";
htmlPage += "      if (!response.ok) throw new Error('Request failed');";
htmlPage += "      fetchStopwatchStatus();";
htmlPage += "    })";
htmlPage += "    .catch(error => showNotification('Stopwatch action failed: ' + error));";
htmlPage += "}";

htmlPage += "window.addEventListener('load', function() {";
htmlPage += "  var clockTab = document.getElementById('clockTab');";
htmlPage += "  if (clockTab) { clockTab.click(); }";
htmlPage += "});";

htmlPage += "setInterval(function() {";
htmlPage += "  fetchCurrentDateTime();";
htmlPage += "  fetchDiagnostics();";
htmlPage += "  fetchColors();";
htmlPage += "  fetchCurrentBrightness();";
htmlPage += "  fetchBrightnessOffsets();";
htmlPage += "}, 5000);";  // Every 5 seconds, fetch updates only when not selecting a color

htmlPage += "setInterval(fetchStopwatchStatus, 1000);";
htmlPage += "fetchStopwatchStatus();";
htmlPage += "fetchDiagnostics();";


  htmlPage += "</script>";

  htmlPage += "</body></html>";

  server.send(200, "text/html", htmlPage);
}

void initWebServer() {
  if (!fsReady) {
    fsReady = LittleFS.begin();
  }
  // Handle CORS preflight and not found requests globally
  server.onNotFound(handleNotFound);

  // Define actual routes
  server.on("/", handleRoot);
  server.on("/controller", handleController);
  server.on("/getClockList", []() { addGlobalCORSHeaders(); handleGetClockList(); });
  server.on("/getDeviceName", []() { addGlobalCORSHeaders(); handleGetDeviceName(); });
  server.on("/getSoftwareVersion", []() { addGlobalCORSHeaders(); handleGetSoftwareVersion(); });
  server.on("/getStatus", []() { addGlobalCORSHeaders(); handleGetStatus(); });
  server.on("/getCurrentMode", []() { addGlobalCORSHeaders(); handleGetCurrentMode(); });
  server.on("/getDiagnostics", []() { addGlobalCORSHeaders(); handleGetDiagnostics(); });
  server.on("/getDiagnosticsSummary", []() { addGlobalCORSHeaders(); handleGetDiagnosticsSummary(); });
  server.on("/getSunrise", []() { addGlobalCORSHeaders(); handleGetSunrise(); });
  server.on("/getSunset", []() { addGlobalCORSHeaders(); handleGetSunset(); });

  server.on("/setClockMode", []() { addGlobalCORSHeaders(); setClockMode(); });
  server.on("/setStopwatchMode", []() { addGlobalCORSHeaders(); setStopwatchMode(); });
  server.on("/setRainbowMode", []() { addGlobalCORSHeaders(); handleRainbowMode(); });
  server.on("/setRainbowClockMode", []() { addGlobalCORSHeaders(); handleRainbowClockMode(); });
  server.on("/setLoveMode", []() { addGlobalCORSHeaders(); handleLoveMode(); });
  server.on("/setFoodMode", []() { addGlobalCORSHeaders(); handleFoodMode(); });

  // Stopwatch control
  server.on("/startStopwatch", []() { addGlobalCORSHeaders(); startStopwatch(); });
  server.on("/stopStopwatch", []() { addGlobalCORSHeaders(); stopStopwatch(); });
  server.on("/resetStopwatch", []() { addGlobalCORSHeaders(); resetStopwatch(); });
  server.on("/addStopwatchMinute", []() { addGlobalCORSHeaders(); addStopwatchMinute(); });
  server.on("/getStopwatchStatus", []() { addGlobalCORSHeaders(); getStopwatchStatus(); });

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
void handleGetStatus() {
  time_t currentEpochTime = timeClient.getEpochTime();
  formatLocalDateTime(currentEpochTime, statusTimeStr, sizeof(statusTimeStr));

  SunriseSunsetTimes times = getSunriseSunsetTimes();
  formatTime12h(times.sunrise, statusSunriseStr, sizeof(statusSunriseStr));
  formatTime12h(times.sunset, statusSunsetStr, sizeof(statusSunsetStr));
  snprintf(statusHourColorStr, sizeof(statusHourColorStr), "#%06X", hourColor & 0xFFFFFF);
  snprintf(statusMinuteColorStr, sizeof(statusMinuteColorStr), "#%06X", minuteColor & 0xFFFFFF);

  int n = snprintf(
      statusResponse, sizeof(statusResponse),
      "{\"deviceName\":\"%s\",\"softwareVersion\":\"%s\",\"currentTime\":\"%s\","
      "\"sunriseTime\":\"%s\",\"sunsetTime\":\"%s\",\"currentMode\":\"%s\","
      "\"currentBrightness\":%u,\"hourColor\":\"%s\",\"minuteColor\":\"%s\"}",
      currentConfig.deviceName,
      softwareVersion,
      statusTimeStr,
      statusSunriseStr,
      statusSunsetStr,
      getClockModeString(currentMode),
      (unsigned)currentBrightness,
      statusHourColorStr,
      statusMinuteColorStr);

  if (n < 0 || n >= (int)sizeof(statusResponse)) {
    server.send(500, "text/plain", "Status JSON too large");
    return;
  }
  server.send(200, "application/json", statusResponse);
}
void handleGetCurrentMode() {
  server.send(200, "text/plain", getClockModeString(currentMode));
}

void handleGetDiagnosticsSummary() {
  diagnosticsRequestCount++;
  ensureResetInfoCached();

  unsigned long uptimeSeconds = millis() / 1000;
  wl_status_t wifiStatus = WiFi.status();
  uint32_t freeHeap = ESP.getFreeHeap();
  uint16_t freeContStack = ESP.getFreeContStack();

  strncpy(diagResetInfoShort, diagResetInfo, sizeof(diagResetInfoShort) - 1);
  diagResetInfoShort[sizeof(diagResetInfoShort) - 1] = '\0';

  // Keep summary intentionally small for periodic polling.
  int n = snprintf(
      diagSummaryResponse, sizeof(diagSummaryResponse),
      "{\"softwareVersion\":\"%s\",\"deviceName\":\"%s\",\"uptimeSeconds\":%lu,"
      "\"freeHeap\":%lu,\"freeContStack\":%u,\"diagnosticsRequestCount\":%lu,"
      "\"wifiStatusCode\":%d,\"wifiStatus\":\"%s\",\"resetReason\":\"%s\",\"resetInfo\":\"%s\"}",
      softwareVersion,
      currentConfig.deviceName,
      uptimeSeconds,
      (unsigned long)freeHeap,
      (unsigned)freeContStack,
      (unsigned long)diagnosticsRequestCount,
      (int)wifiStatus,
      wifiStatusToString(wifiStatus),
      diagResetReason,
      diagResetInfoShort);

  if (n < 0 || n >= (int)sizeof(diagSummaryResponse)) {
    server.send(500, "text/plain", "Diagnostics summary JSON too large");
    return;
  }
  server.send(200, "application/json", diagSummaryResponse);
}

void handleGetDiagnostics() {
  diagnosticsRequestCount++;
  ensureResetInfoCached();

  // Rebuild expensive diagnostics payload at most once per short interval.
  // Repeated browser polling then mostly serves cached JSON.
  unsigned long nowMs = millis();
  if ((nowMs - diagLastBuildMs) < DIAG_BUILD_INTERVAL_MS && diagResponse[0] != '\0') {
    server.send(200, "application/json", diagResponse);
    return;
  }

  unsigned long uptimeSeconds = millis() / 1000;
  time_t nowEpoch = timeClient.getEpochTime();
  bool timeSynced = (nowEpoch > 1700000000);  // Basic guard against unsynced NTP time.
  wl_status_t wifiStatus = WiFi.status();
  uint32_t freeHeap = ESP.getFreeHeap();
  uint8_t heapFrag = ESP.getHeapFragmentation();
  uint32_t maxFreeBlock = ESP.getMaxFreeBlockSize();
  uint16_t freeContStack = ESP.getFreeContStack();
  String ssidStr = WiFi.SSID();
  ssidStr.toCharArray(diagSsid, sizeof(diagSsid));

  IPAddress ip = WiFi.localIP();
  IPAddress subnet = WiFi.subnetMask();
  IPAddress gateway = WiFi.gatewayIP();
  IPAddress dns = WiFi.dnsIP();

  snprintf(diagIpStr, sizeof(diagIpStr), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  snprintf(diagSubnetStr, sizeof(diagSubnetStr), "%u.%u.%u.%u", subnet[0], subnet[1], subnet[2], subnet[3]);
  snprintf(diagGatewayStr, sizeof(diagGatewayStr), "%u.%u.%u.%u", gateway[0], gateway[1], gateway[2], gateway[3]);
  snprintf(diagDnsStr, sizeof(diagDnsStr), "%u.%u.%u.%u", dns[0], dns[1], dns[2], dns[3]);

  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(diagMacStr, sizeof(diagMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  long rssi = WiFi.RSSI();

  int n = 0;

  if (timeSynced) {
    formatLocalDateTime(nowEpoch, diagNowTs, sizeof(diagNowTs));

    if (nowEpoch > (time_t)uptimeSeconds) {
      time_t estimatedResetEpoch = nowEpoch - (time_t)uptimeSeconds;
      formatLocalDateTime(estimatedResetEpoch, diagResetTs, sizeof(diagResetTs));

      n = snprintf(
          diagResponse, sizeof(diagResponse),
          "{\"uptimeSeconds\":%lu,\"freeHeap\":%lu,\"minFreeHeapSinceBoot\":%lu,"
          "\"heapFragmentation\":%u,\"maxFreeBlockSize\":%lu,\"freeContStack\":%u,"
          "\"softwareVersion\":\"%s\",\"deviceName\":\"%s\",\"selectedClock\":%d,"
          "\"currentMode\":\"%s\",\"currentModeId\":%d,\"diagnosticsRequestCount\":%lu,"
          "\"wifiStatusCode\":%d,\"wifiStatus\":\"%s\",\"wifiConnected\":%s,"
          "\"ip\":\"%s\",\"subnetMask\":\"%s\",\"gateway\":\"%s\",\"dns\":\"%s\","
          "\"mac\":\"%s\",\"ssid\":\"%s\",\"rssi\":%ld,\"resetReason\":\"%s\","
          "\"resetInfo\":\"%s\",\"timeSynced\":true,\"currentEpoch\":%ld,"
          "\"currentLocalTime\":\"%s\",\"estimatedResetEpoch\":%ld,"
          "\"estimatedResetLocalTime\":\"%s\"}",
          uptimeSeconds, (unsigned long)freeHeap, (unsigned long)minFreeHeapSinceBoot,
          (unsigned)heapFrag, (unsigned long)maxFreeBlock, (unsigned)freeContStack,
          softwareVersion, currentConfig.deviceName, selectedClock,
          getClockModeString(currentMode), (int)currentMode, (unsigned long)diagnosticsRequestCount,
          (int)wifiStatus, wifiStatusToString(wifiStatus), (wifiStatus == WL_CONNECTED) ? "true" : "false",
          diagIpStr, diagSubnetStr, diagGatewayStr, diagDnsStr,
          diagMacStr, diagSsid, rssi, diagResetReason, diagResetInfo,
          (long)nowEpoch, diagNowTs, (long)estimatedResetEpoch, diagResetTs);
    } else {
      n = snprintf(
          diagResponse, sizeof(diagResponse),
          "{\"uptimeSeconds\":%lu,\"freeHeap\":%lu,\"minFreeHeapSinceBoot\":%lu,"
          "\"heapFragmentation\":%u,\"maxFreeBlockSize\":%lu,\"freeContStack\":%u,"
          "\"softwareVersion\":\"%s\",\"deviceName\":\"%s\",\"selectedClock\":%d,"
          "\"currentMode\":\"%s\",\"currentModeId\":%d,\"diagnosticsRequestCount\":%lu,"
          "\"wifiStatusCode\":%d,\"wifiStatus\":\"%s\",\"wifiConnected\":%s,"
          "\"ip\":\"%s\",\"subnetMask\":\"%s\",\"gateway\":\"%s\",\"dns\":\"%s\","
          "\"mac\":\"%s\",\"ssid\":\"%s\",\"rssi\":%ld,\"resetReason\":\"%s\","
          "\"resetInfo\":\"%s\",\"timeSynced\":true,\"currentEpoch\":%ld,"
          "\"currentLocalTime\":\"%s\"}",
          uptimeSeconds, (unsigned long)freeHeap, (unsigned long)minFreeHeapSinceBoot,
          (unsigned)heapFrag, (unsigned long)maxFreeBlock, (unsigned)freeContStack,
          softwareVersion, currentConfig.deviceName, selectedClock,
          getClockModeString(currentMode), (int)currentMode, (unsigned long)diagnosticsRequestCount,
          (int)wifiStatus, wifiStatusToString(wifiStatus), (wifiStatus == WL_CONNECTED) ? "true" : "false",
          diagIpStr, diagSubnetStr, diagGatewayStr, diagDnsStr,
          diagMacStr, diagSsid, rssi, diagResetReason, diagResetInfo,
          (long)nowEpoch, diagNowTs);
    }
  } else {
    n = snprintf(
        diagResponse, sizeof(diagResponse),
        "{\"uptimeSeconds\":%lu,\"freeHeap\":%lu,\"minFreeHeapSinceBoot\":%lu,"
        "\"heapFragmentation\":%u,\"maxFreeBlockSize\":%lu,\"freeContStack\":%u,"
        "\"softwareVersion\":\"%s\",\"deviceName\":\"%s\",\"selectedClock\":%d,"
        "\"currentMode\":\"%s\",\"currentModeId\":%d,\"diagnosticsRequestCount\":%lu,"
        "\"wifiStatusCode\":%d,\"wifiStatus\":\"%s\",\"wifiConnected\":%s,"
        "\"ip\":\"%s\",\"subnetMask\":\"%s\",\"gateway\":\"%s\",\"dns\":\"%s\","
        "\"mac\":\"%s\",\"ssid\":\"%s\",\"rssi\":%ld,\"resetReason\":\"%s\","
        "\"resetInfo\":\"%s\",\"timeSynced\":false}",
        uptimeSeconds, (unsigned long)freeHeap, (unsigned long)minFreeHeapSinceBoot,
        (unsigned)heapFrag, (unsigned long)maxFreeBlock, (unsigned)freeContStack,
        softwareVersion, currentConfig.deviceName, selectedClock,
        getClockModeString(currentMode), (int)currentMode, (unsigned long)diagnosticsRequestCount,
        (int)wifiStatus, wifiStatusToString(wifiStatus), (wifiStatus == WL_CONNECTED) ? "true" : "false",
        diagIpStr, diagSubnetStr, diagGatewayStr, diagDnsStr,
        diagMacStr, diagSsid, rssi, diagResetReason, diagResetInfo);
  }

  if (n < 0 || n >= (int)sizeof(diagResponse)) {
    server.send(500, "text/plain", "Diagnostics JSON too large");
    return;
  }
  diagLastBuildMs = nowMs;
  server.send(200, "application/json", diagResponse);
}
void handleGetSunrise(){
    SunriseSunsetTimes times = getSunriseSunsetTimes();
    char sunriseStr[16];
    formatTime12h(times.sunrise, sunriseStr, sizeof(sunriseStr));
    server.send(200, "text/plain", sunriseStr);
}
void handleGetSunset(){
    SunriseSunsetTimes times = getSunriseSunsetTimes();
    char sunsetStr[16];
    formatTime12h(times.sunset, sunsetStr, sizeof(sunsetStr));
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
  stopwatchStop();
  server.send(200, "text/plain", "Stopwatch mode set");
}

void handleRainbowMode() {
  currentMode = RAINBOW_MODE;
  server.send(200, "text/html", "Switched to Rainbow Mode");
}

void handleRainbowClockMode() {
  currentMode = RAINBOW_CLOCK_MODE;
  server.send(200, "text/html", "Switched to Rainbow Clock Mode");
}

void handleLoveMode() {
  currentMode = LOVE_MODE;
  server.send(200, "text/html", "Switched to Love Mode");
}

void handleFoodMode() {
  currentMode = FOOD_MODE;
  server.send(200, "text/html", "Switched to Food Mode");
}

void startStopwatch() {
  stopwatchStart();
  server.send(200, "text/plain", "Stopwatch started");
}

void stopStopwatch() {
  stopwatchStop();
  server.send(200, "text/plain", "Stopwatch stopped");
}

void resetStopwatch() {
  stopwatchReset();
  server.send(200, "text/plain", "Stopwatch reset");
}

void addStopwatchMinute() {
  stopwatchAddMinute();
  server.send(200, "text/plain", "Stopwatch minute added");
}

void getStopwatchStatus() {
  unsigned long remainingMs = stopwatchGetRemainingMs();
  int n = snprintf(
      smallJsonResponse, sizeof(smallJsonResponse),
      "{\"running\":%s,\"remainingMs\":%lu,\"remainingSeconds\":%lu}",
      stopwatchIsRunning() ? "true" : "false",
      remainingMs,
      (remainingMs + 999UL) / 1000UL);
  if (n < 0 || n >= (int)sizeof(smallJsonResponse)) {
    server.send(500, "text/plain", "Stopwatch JSON too large");
    return;
  }
  server.send(200, "application/json", smallJsonResponse);
}

void resetColors() {
  hourColor = currentConfig.DEFAULT_HOUR_COLOR;
  minuteColor = currentConfig.DEFAULT_MINUTE_COLOR;
  colonColor = currentConfig.DEFAULT_COLON_COLOR;

  char hourColorStr[8], minuteColorStr[8], colonColorStr[8];
  snprintf(hourColorStr, sizeof(hourColorStr), "#%06X", currentConfig.DEFAULT_HOUR_COLOR & 0xFFFFFF);
  snprintf(minuteColorStr, sizeof(minuteColorStr), "#%06X", currentConfig.DEFAULT_MINUTE_COLOR & 0xFFFFFF);
  snprintf(colonColorStr, sizeof(colonColorStr), "#%06X", currentConfig.DEFAULT_COLON_COLOR & 0xFFFFFF);

  int n = snprintf(
      smallJsonResponse, sizeof(smallJsonResponse),
      "{\"defaultHourColor\":\"%s\",\"defaultMinuteColor\":\"%s\",\"defaultColonColor\":\"%s\"}",
      hourColorStr, minuteColorStr, colonColorStr);
  if (n < 0 || n >= (int)sizeof(smallJsonResponse)) {
    server.send(500, "text/plain", "Color JSON too large");
    return;
  }
  server.send(200, "application/json", smallJsonResponse);
}

void getBrightness() {
  int n = snprintf(smallJsonResponse, sizeof(smallJsonResponse), "{\"brightness\":%u}", (unsigned)currentBrightness);
  if (n < 0 || n >= (int)sizeof(smallJsonResponse)) {
    server.send(500, "text/plain", "Brightness JSON too large");
    return;
  }
  server.send(200, "application/json", smallJsonResponse);
}

void setBrightnessOffsets() {
  if (server.hasArg("plain")) {
    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));

    if (doc["dayOffset"].is<int>()) {
      dayTimeBrightnessOffset = doc["dayOffset"].as<int>();
    }
    if (doc["nightOffset"].is<int>()) {
      nightTimeBrightnessOffset = doc["nightOffset"].as<int>();
    }
    server.send(200, "text/html", "Brightness offsets set successfully.");
  } else {
    server.send(400, "text/html", "Missing parameters.");
  }
}

void handleGetHourColor() {
  char colorStr[8];
  snprintf(colorStr, sizeof(colorStr), "#%06X", hourColor & 0xFFFFFF);
  server.send(200, "text/plain", colorStr);
}

void handleGetMinuteColor() {
  char colorStr[8];
  snprintf(colorStr, sizeof(colorStr), "#%06X", minuteColor & 0xFFFFFF);
  server.send(200, "text/plain", colorStr);
}

void handleGetColonColor() {
  char colorStr[8];
  snprintf(colorStr, sizeof(colorStr), "#%06X", colonColor & 0xFFFFFF);
  server.send(200, "text/plain", colorStr);
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
  formatLocalDateTime(currentEpochTime, genericTimeStr, sizeof(genericTimeStr));
  server.send(200, "text/plain", genericTimeStr);
}

void getBrightnessOffsets() {
  int n = snprintf(
      smallJsonResponse, sizeof(smallJsonResponse),
      "{\"dayOffset\":%d,\"nightOffset\":%d}",
      dayTimeBrightnessOffset, nightTimeBrightnessOffset);
  if (n < 0 || n >= (int)sizeof(smallJsonResponse)) {
    server.send(500, "text/plain", "Brightness offset JSON too large");
    return;
  }
  server.send(200, "application/json", smallJsonResponse);
}
