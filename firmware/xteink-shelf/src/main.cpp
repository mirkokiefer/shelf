/**
 * Shelf — Daslab E-Ink Dashboard for Xteink X4
 * 
 * Connects to WiFi, fetches a server-rendered PNG image,
 * and displays it on the X4's e-paper screen.
 * 
 * The server (Bun + Satori) renders beautiful dashboard layouts
 * as 480×800 PNG images. This firmware just fetches and displays them.
 * 
 * Modes (cycled via page buttons):
 *   - dashboard: time, GitHub stats, deploy status, device list
 *   - quote: centered inspirational quote
 *   - status: system status overview
 * 
 * Part of the Daslab Shelf project.
 * https://github.com/mirkokiefer/shelf
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Include the open-x4-sdk display controller
// If building without the SDK, you can stub these out
#ifdef USE_EPD_SDK
#include <EpdScreenController.h>
#include <BatteryMonitor.h>
#endif

#include "config.h"

// ─── State ───
static const char* modes[] = {"dashboard", "quote", "status"};
static int currentMode = 0;
static const int NUM_MODES = 3;
static unsigned long lastRefresh = 0;
static bool needsRefresh = true;

// ─── WiFi ───
void connectWiFi() {
  Serial.printf("Connecting to WiFi: %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

// ─── Fetch image from server ───
// Returns a buffer with PNG data, caller must free()
uint8_t* fetchImage(const char* url, size_t* outSize) {
  HTTPClient http;
  http.begin(url);
  http.setTimeout(15000);
  
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("HTTP GET failed: %d\n", httpCode);
    http.end();
    *outSize = 0;
    return nullptr;
  }
  
  size_t len = http.getSize();
  if (len <= 0 || len > 500000) {
    Serial.printf("Invalid content length: %d\n", len);
    http.end();
    *outSize = 0;
    return nullptr;
  }
  
  uint8_t* buffer = (uint8_t*)malloc(len);
  if (!buffer) {
    Serial.println("Failed to allocate memory for image");
    http.end();
    *outSize = 0;
    return nullptr;
  }
  
  WiFiClient* stream = http.getStreamPtr();
  size_t bytesRead = 0;
  while (bytesRead < len) {
    size_t available = stream->available();
    if (available) {
      size_t toRead = min(available, len - bytesRead);
      size_t read = stream->readBytes(buffer + bytesRead, toRead);
      bytesRead += read;
    }
    delay(1);
  }
  
  http.end();
  *outSize = bytesRead;
  Serial.printf("Downloaded %d bytes\n", bytesRead);
  return buffer;
}

// ─── Set mode on server ───
void setServerMode(const char* mode) {
  HTTPClient http;
  String url = String(SHELF_SERVER_URL) + "/api/data";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  String body = "{\"mode\":\"";
  body += mode;
  body += "\"}";
  
  int httpCode = http.POST(body);
  Serial.printf("Set mode to '%s': HTTP %d\n", mode, httpCode);
  http.end();
}

// ─── Display image ───
void displayImage(uint8_t* imageData, size_t imageSize) {
#ifdef USE_EPD_SDK
  // Use the open-x4-sdk EpdScreenController to display the image
  // The SDK handles SPI communication with the e-paper display
  EpdScreenController epd;
  epd.init();
  epd.displayPng(imageData, imageSize);
  epd.sleep();
#else
  // Stub: just log that we would display
  Serial.printf("Would display %d bytes of image data on e-paper\n", imageSize);
  Serial.println("(Build with -DUSE_EPD_SDK to enable actual display)");
#endif
}

// ─── Refresh display ───
void refreshDisplay() {
  String url = String(SHELF_SERVER_URL) + "/eink.png";
  Serial.printf("Fetching: %s\n", url.c_str());
  
  size_t imageSize = 0;
  uint8_t* imageData = fetchImage(url.c_str(), &imageSize);
  
  if (imageData && imageSize > 0) {
    displayImage(imageData, imageSize);
    free(imageData);
    Serial.println("Display updated!");
  } else {
    Serial.println("Failed to fetch image");
  }
  
  lastRefresh = millis();
  needsRefresh = false;
}

// ─── Button handling ───
void checkButtons() {
  // Page forward = next mode
  if (digitalRead(BTN_NEXT) == LOW) {
    delay(50); // debounce
    if (digitalRead(BTN_NEXT) == LOW) {
      currentMode = (currentMode + 1) % NUM_MODES;
      Serial.printf("Mode: %s\n", modes[currentMode]);
      setServerMode(modes[currentMode]);
      delay(500); // let server render
      needsRefresh = true;
      while (digitalRead(BTN_NEXT) == LOW) delay(10); // wait release
    }
  }
  
  // Page back = previous mode
  if (digitalRead(BTN_PREV) == LOW) {
    delay(50);
    if (digitalRead(BTN_PREV) == LOW) {
      currentMode = (currentMode - 1 + NUM_MODES) % NUM_MODES;
      Serial.printf("Mode: %s\n", modes[currentMode]);
      setServerMode(modes[currentMode]);
      delay(500);
      needsRefresh = true;
      while (digitalRead(BTN_PREV) == LOW) delay(10);
    }
  }
  
  // OK button = force refresh
  if (digitalRead(BTN_OK) == LOW) {
    delay(50);
    if (digitalRead(BTN_OK) == LOW) {
      Serial.println("Force refresh");
      needsRefresh = true;
      while (digitalRead(BTN_OK) == LOW) delay(10);
    }
  }
}

// ─── Setup ───
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("┌──────────────────────────────┐");
  Serial.println("│  Shelf — Daslab E-Ink Widget  │");
  Serial.println("│  Xteink X4 · 480×800         │");
  Serial.println("└──────────────────────────────┘");
  Serial.println();
  
  // Setup buttons
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_OK,   INPUT_PULLUP);
  
  // Connect WiFi
  connectWiFi();
  
  if (WiFi.status() == WL_CONNECTED) {
    // Initial display refresh
    refreshDisplay();
  }
}

// ─── Main Loop ───
void loop() {
  // Check for button presses
  checkButtons();
  
  // Auto-refresh on interval
  if (millis() - lastRefresh >= REFRESH_INTERVAL_MS) {
    needsRefresh = true;
  }
  
  // Refresh if needed and WiFi is connected
  if (needsRefresh && WiFi.status() == WL_CONNECTED) {
    refreshDisplay();
  }
  
  // Reconnect WiFi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    connectWiFi();
  }
  
#if USE_DEEP_SLEEP
  // Deep sleep mode — wake on timer
  Serial.println("Going to deep sleep...");
  esp_sleep_enable_timer_wakeup(REFRESH_INTERVAL_MS * 1000ULL);
  esp_deep_sleep_start();
#else
  delay(100);
#endif
}
