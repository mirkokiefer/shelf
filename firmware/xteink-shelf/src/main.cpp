/**
 * Shelf — Daslab E-Ink Dashboard for Xteink X4
 *
 * Fetches a server-rendered 480x800 grayscale image,
 * pre-dithered to 1-bit on the server, streams directly
 * into the e-paper framebuffer. Zero decoding on device.
 *
 * Uses open-x4-sdk for display, buttons, battery.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <EInkDisplay.h>
#include <InputManager.h>
#include <BatteryMonitor.h>
#include "config.h"
// Arduino Core 3.x + ARDUINO_USB_CDC_ON_BOOT=1: Serial maps to USB CDC
#define LOG Serial

// ─── Hardware ───
// X4 pin mapping
EInkDisplay display(EPD_SCLK, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
InputManager buttons;

// Battery ADC pin (X4 uses GPIO2 with voltage divider)
#define BATTERY_ADC_PIN 2
BatteryMonitor battery(BATTERY_ADC_PIN);

// ─── State ───
static const char* modes[] = {"dashboard", "quote", "status"};
static int currentMode = 0;
static const int NUM_MODES = 3;
static unsigned long lastRefresh = 0;
static bool needsRefresh = true;

// ─── WiFi ───
void connectWiFi() {
  LOG.printf("Connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    LOG.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    LOG.printf("\nConnected! IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    LOG.println("\nWiFi connection failed!");
  }
}

// ─── Set mode on server ───
void setServerMode(const char* mode) {
  HTTPClient http;
  String url = String(SHELF_SERVER_URL) + "/api/data";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  String body = String("{\"mode\":\"") + mode + "\"}";
  int code = http.POST(body);
  LOG.printf("Set mode '%s': %d\n", mode, code);
  http.end();
}

// ─── Fetch raw 1-bit framebuffer from server and display ───
bool fetchAndDisplay() {
  String url = String(SHELF_SERVER_URL) + "/eink.raw";
  LOG.printf("Fetching: %s\n", url.c_str());

  HTTPClient http;
  http.begin(url);
  http.setTimeout(15000);
  int httpCode = http.GET();

  if (httpCode != 200) {
    LOG.printf("HTTP error: %d\n", httpCode);
    http.end();
    return false;
  }

  int len = http.getSize();
  const uint32_t expectedSize = EInkDisplay::BUFFER_SIZE; // 800*480/8 = 48000

  if (len != (int)expectedSize) {
    LOG.printf("Unexpected size: %d (expected %lu)\n", len, expectedSize);
    http.end();
    return false;
  }

  // Stream directly into the display framebuffer — zero copy!
  uint8_t* fb = display.getFrameBuffer();
  WiFiClient* stream = http.getStreamPtr();
  size_t bytesRead = 0;

  while (bytesRead < expectedSize) {
    size_t avail = stream->available();
    if (avail) {
      size_t toRead = min(avail, (size_t)(expectedSize - bytesRead));
      size_t got = stream->readBytes(fb + bytesRead, toRead);
      bytesRead += got;
    }
    delay(1);
  }

  http.end();
  LOG.printf("Downloaded %d bytes into framebuffer\n", bytesRead);

  // Refresh the e-paper display
  display.displayBuffer(EInkDisplay::FAST_REFRESH, true);
  LOG.println("Display updated!");

  lastRefresh = millis();
  needsRefresh = false;
  return true;
}

// ─── Setup ───
void setup() {
  LOG.begin(115200);
  delay(500);

  LOG.println();
  LOG.println("================================");
  LOG.println("  Shelf - Daslab E-Ink Widget");
  LOG.println("  Xteink X4 - 480x800");
  LOG.println("================================");

  // Init hardware
  display.begin();
  buttons.begin();

  LOG.printf("Battery: %d%%\n", battery.readPercentage());

  // Connect WiFi
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    display.clearScreen(0xFF);
    display.displayBuffer(EInkDisplay::FAST_REFRESH, false);
    fetchAndDisplay();
  }
}

// ─── Loop ───
void loop() {
  buttons.update();

  // Right = next mode
  if (buttons.wasPressed(3)) {
    currentMode = (currentMode + 1) % NUM_MODES;
    LOG.printf("Mode -> %s\n", modes[currentMode]);
    setServerMode(modes[currentMode]);
    delay(500);
    needsRefresh = true;
  }

  // Left = prev mode
  if (buttons.wasPressed(2)) {
    currentMode = (currentMode - 1 + NUM_MODES) % NUM_MODES;
    LOG.printf("Mode -> %s\n", modes[currentMode]);
    setServerMode(modes[currentMode]);
    delay(500);
    needsRefresh = true;
  }

  // Confirm = force refresh
  if (buttons.wasPressed(1)) {
    LOG.println("Force refresh");
    needsRefresh = true;
  }

  // Auto-refresh
  if (millis() - lastRefresh >= REFRESH_INTERVAL_MS) {
    needsRefresh = true;
  }

  if (needsRefresh && WiFi.status() == WL_CONNECTED) {
    fetchAndDisplay();
  }

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  delay(50);
}
