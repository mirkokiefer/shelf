#pragma once

// ─── WiFi ───
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// ─── Server URL (must be reachable from X4 on same network) ───
#define SHELF_SERVER_URL "http://192.168.1.100:3001"

// ─── Refresh interval ───
#define REFRESH_INTERVAL_MS 60000  // 60 seconds

// ─── X4 display pin mapping (from CrossPoint/open-x4-sdk) ───
#define EPD_SCLK  4
#define EPD_MOSI  6
#define EPD_CS    7
#define EPD_DC    5
#define EPD_RST   1
#define EPD_BUSY  0
