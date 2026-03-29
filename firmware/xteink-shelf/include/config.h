#pragma once

// ─── WiFi Configuration ───
// Override these via build_flags in platformio.ini or set them here
#ifndef WIFI_SSID
#define WIFI_SSID "YOUR_WIFI_SSID"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#endif

// ─── Server Configuration ───
// URL of the Shelf server that renders e-ink images
#ifndef SHELF_SERVER_URL
#define SHELF_SERVER_URL "http://YOUR_SERVER_IP:3001"
#endif

// ─── Display Configuration ───
#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 800

// ─── Refresh interval (ms) ───
// How often to fetch a new image from the server
#define REFRESH_INTERVAL_MS 60000  // 60 seconds

// ─── Deep sleep between refreshes ───
// Set to true to use deep sleep (saves battery)
// Set to false for continuous operation (faster updates)
#define USE_DEEP_SLEEP false

// ─── Button Pins (Xteink X4) ───
#define BTN_PREV   8   // Page back
#define BTN_NEXT   10  // Page forward
#define BTN_OK     3   // Confirm / Menu
#define BTN_POWER  9   // Power button
