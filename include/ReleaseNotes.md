# Release Notes
/*
Version 0.212 - February 7, 2026
- Implemented stopwatch mode in firmware:
  - Countdown render in `MM:SS` on the LED display.
  - Added stopwatch state/actions: start, stop, reset, add 1 minute.
  - Added zero-finish behavior: blink display 5 times, then keep showing `00:00`.
- Added stopwatch API endpoints:
  - `/startStopwatch`, `/stopStopwatch`, `/resetStopwatch`, `/addStopwatchMinute`, `/getStopwatchStatus`.
- Updated device web UI (`/`) stopwatch tab:
  - Added working buttons for start/stop/reset/add minute.
  - Added live stopwatch status display.
- Updated controller page (`/controller`):
  - Added `Set Stopwatch Mode` as mode action.
  - Added stopwatch action buttons for selected clock/all clocks.
  - Stopwatch control section is shown only when one clock is selected and that clock is in `Stopwatch Mode`.
  - Added diagnostics status cell per clock using `/getDiagnostics`.
  - Diagnostics are color-coded: green for normal/system restart, red for crash-like reset info.
- Upload helper (`tools/upload.py`) was simplified again for speed:
  - Removed heavy pre-upload hostname discovery checks.
  - Uses direct target from `ota_host` or `<clockname>.local`.
  - Keeps menu flow responsive before upload starts.

Version 0.211 - February 6, 2026
- Migrated project to PlatformIO structure (src/, include/, platformio.ini).
- Added OTA support (ArduinoOTA) and mDNS-based uploads using <deviceName>.local.
- Added upload helper scripts (tools/upload.py, tools/ports.py) to select clock and upload method.
- Wi‑Fi credentials are now per‑clock in clockConfigs (Settings.cpp).
- Controller is served from LittleFS (`/controller`) and uses `/getClockList` with mDNS hosts.
- Controller now fetches per-device data through `/getStatus` (JSON).
- Controller now shows only reachable clocks.
- Controller now auto-selects the current device and includes a probe-target debug view.
- Controller hostname mapping now uses lowercase underscore-based `.local` names.
- Upload helper can do firmware, filesystem, or both with OTA wait/retry.
- Added `/getDiagnostics` endpoint (uptime, heap, reset reason/info).
- Rainbow/Food rendering is now non-blocking for better OTA/web responsiveness.
- Added periodic Wi‑Fi reconnect logic in main loop.
- Added bounds-check/fallback for `selectedClock`.
- Updated clock display to apply runtime hour/minute color changes.
- Improved sunrise/sunset handling with DST-aware offset.
- Updated warnings/cleanup from the PlatformIO migration.

Version 0.210 - 27 October 2024
- Fixed bug for wintertime

Version 0.209 - 12 September 2024

New Features:
- Controller Page:
  - A completely redesigned controller interface for managing multiple clocks.
  - The controller page allows users to select a clock from a dropdown and control different modes (Clock, Love, Food, Rainbow) for each selected clock.
  - The device information table shows detailed information (Device Name, Software Version, Current Time, Sunrise/Sunset, Current Mode, Current Brightness, Hour Color, Minute Color) for all clocks.
  - Device names in the table header are clickable links that redirect to the respective clock’s IP address.

Bug Fixes:
- Fixed issues with brightness sliders and color pickers not properly syncing with the selected clock’s values.


Version 0.208 - 12 September 2024

Improvements:
- Enhanced Color Picker Handling:
  - Implemented a feature to temporarily pause color picker updates during user interaction. 
    When a user selects a new color, the current clock color is immediately updated, but the 
    color picker values do not reset due to the periodic server fetches.
  - Introduced logic to allow the user’s selected color to be applied immediately on the clock display.
  - Cleaned out variables

- Color Input Fields:
  - Initial values for the color pickers (Hour, Minute, Colon) are now properly set to match the 
    current clock configuration using the `#RRGGBB` format.

Bug Fixes:
- Day/Night Brightness Offset:
  - Resolved an issue where the daytime and nighttime offset sliders were resetting to default 
    values after being changed. Slider values are now properly sent to and processed by the server.

## Version 0.207 - September 5, 2024
    ### New Features:
    - **Colon Color Picker Added**:
      - A new color picker has been added for the colon display in the web interface.
      - Users can now select a custom color for the colon, and it updates dynamically on the clock.

    ### Improvements:
    - **Consistent Color Handling**:
      - Fixed an issue where the colon color was incorrectly returned as a decimal value. Now, the colon color is returned in `#RRGGBB` format, consistent with the hour and minute colors.
      - All color pickers (hour, minute, and colon) now work uniformly, and selected values are properly saved and displayed without resetting to defaults.

    ### Bug Fixes:
    - **Color Picker Default Reset**:
      - Fixed an issue where the color picker for the colon reverted to black on page load despite the color being set to yellow (`0xFFFF00`).
      - Resolved an issue where selected colors would reset to default values after a new color was picked.

    ### Known Issues:
    - Stopwatch functionality is not fully implemented yet in the web interface.


### Version 0.206 - September 5, 2024
    #### New Features:
    - **Mobile-Friendly Web Interface**:
      - The web interface has been redesigned to be responsive and usable on mobile devices. This includes:
        - Automatic adjustment of buttons and input fields to screen size.
        - Larger, touch-friendly buttons and inputs.
        - Media queries to optimize the layout for screens smaller than 600px.

    - **Live Updates for Clock and Settings**:
      - The web interface now updates dynamically every 2 seconds without requiring a page refresh. This includes:
        - Current date and time (`dd-mm-yyyy hh:mm:ss`) displayed under the software version.
        - Updates to color pickers for hour and minute colors.
        - Current brightness level and daytime/nighttime brightness offsets.

    #### Improvements:
    - **Responsive Elements**:
      - Input fields and buttons now automatically resize and adjust for better usability on both desktop and mobile devices.
      - Text is center-aligned to improve readability across devices.

    #### Bug Fixes:
    - **Color Reset Fix**:
      - Resolved an issue where the clock's color pickers would reset to black after the "Set to Defaults" button was used.
      - The internal clock colors now reset properly to the default values as well.

## Version 205 - August 31, 2024
    ### New Features:
    - Brightness Control Update:
      - The clock now adjusts its brightness based on ambient light levels:
        - Inverted Light Sensor Handling: Brightness increases with more light and decreases with less light.
        - Minimum Brightness: The clock’s brightness will not drop below 3, ensuring visibility in low light.
        - Daytime and Nighttime Adjustments: Brightness is adjusted based on whether it is daytime or nighttime, using the `isDayTime()` function. Configurable offsets for both.

    - Leading Zero Handling:
      - When the time is between 00:00 and 09:59, the leading zero in the hour display is hidden. For example, 07:30 will be displayed as 7:30.

    - New Modes:
      - Rainbow Mode: Displays a rainbow pattern across the LED strip.
      - Love Mode: Displays the letters "LOVE" across the LED segments.
      - Food Mode: Blinks the word "FOOD" in alternating colors.

    ### Improvements:
    - Web Interface Enhancements:
      - Updated the interface for seamless mode switching and brightness control. Changes are reflected in real-time.

    ### Known Issues:
    - Calibrate the light sensor properly to avoid unexpected brightness adjustments.
    - Ensure offsets are appropriate for your environment to maintain readability day and night.

## Version 0.204 - August 30, 2024
    ### New Features:
    - **Dynamic Web Interface**:
      - Added JavaScript functionality to update time, brightness, and color settings in real-time without needing to refresh the page.
      - Implemented automatic updates for the displayed time and brightness every second and two seconds, respectively.
      - Integrated an API to fetch current time and brightness values dynamically.
      
    - **Sunrise/Sunset Time Calculation**:
      - Improved sunrise and sunset time calculation with proper adjustments for the time zone and daylight saving time (DST).
      - Added debugging outputs to monitor calculated times and adjustments for better troubleshooting.

    ### Improvements:
    - **Web Interface Enhancements**:
      - Removed the need for separate "Set" buttons for brightness offsets and color pickers, allowing for immediate application of changes.
      - Included the current date in the web interface, which updates dynamically upon page load.

    ### Bug Fixes:
    - **Time Zone and DST Handling**:
      - Fixed issues with sunrise and sunset calculations being offset by 2 hours due to improper time zone handling.
      - Corrected the application of DST to ensure accurate local time for sunrise and sunset events.

    ### Known Issues:
    - None reported for this version.

    ### Next Steps:
    - Further refine web interface responsiveness.
    - Consider adding more customization options for different clock modes.

Version 0.203 - August 29, 2024
    •	New Features:
    •	Implemented smooth transitions for LED brightness based on ambient light levels.
    •	Added logic to ensure the clock display adjusts brightness correctly, preventing oscillations when the light sensor reads 0.
    •	Enhanced nighttime display mode: digits now turn red during nighttime.
    •	Improvements:
    •	Updated brightness control to handle low light sensor values gracefully, ensuring the display remains readable in all conditions.

## Version 0.201 - 29-08-2024
    ### Changes and Improvements
    - **Code Refactoring**: Split the code into multiple files for better organization and maintainability.
    - **Web Interface Enhancements**:
      - Added the ability to change hour and minute colors via the web interface.
      - Added controls for adjusting daytime and nighttime brightness offsets.
      - Implemented mode switching via the web interface, including Clock Mode, Stopwatch Mode, Rainbow Mode, Love Mode, and Food Mode.
      - Integrated a stopwatch feature with start, stop, and reset controls accessible from the web interface.
      - Displayed the current software version on the web interface.
      - Added an API response banner that appears on the web interface after API calls, which can be dismissed by the user.
    - **General Bug Fixes**:
      - Resolved compilation issues related to undeclared variables and missing libraries.
      - Ensured that the clock display functions as expected with no runtime errors.

    ### Known Issues
    - None reported for this version.

    ### Next Steps
    - Continued testing to ensure all functionalities work as expected in various scenarios.
    - Consider additional enhancements based on user feedback.

---

## Future versions
- add a small speaker
- add an alarm function
- what to do with the colon in clock mode
