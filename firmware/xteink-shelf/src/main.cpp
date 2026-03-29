/**
 * Shelf — MINIMAL TEST FIRMWARE
 * 
 * Just initializes display and draws a test pattern.
 * No WiFi, no HTTP, no nothing. If this works, display is fine.
 */

#include <Arduino.h>
#include <EInkDisplay.h>
#include "config.h"

#define LOG Serial

// Display
EInkDisplay display(EPD_SCLK, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

void setup() {
  LOG.begin(115200);
  delay(2000); // Give USB CDC time to connect
  
  LOG.println("=== Shelf Test Firmware ===");
  LOG.println("Step 1: Initializing display...");
  
  display.begin();
  LOG.println("Step 2: Display initialized OK");
  
  // Clear to white
  LOG.println("Step 3: Clearing screen to white...");
  display.clearScreen(0xFF);
  display.displayBuffer(EInkDisplay::FULL_REFRESH, false);
  LOG.println("Step 4: Screen cleared");
  
  delay(2000);
  
  // Draw a simple test pattern - half black, half white
  LOG.println("Step 5: Drawing test pattern...");
  uint8_t* fb = display.getFrameBuffer();
  uint32_t bufSize = EInkDisplay::BUFFER_SIZE;
  
  // Top half white (0xFF), bottom half black (0x00)
  memset(fb, 0xFF, bufSize / 2);
  memset(fb + bufSize / 2, 0x00, bufSize / 2);
  
  display.displayBuffer(EInkDisplay::FULL_REFRESH, true);
  LOG.println("Step 6: Test pattern displayed!");
  LOG.println("=== DONE - If you see half white/half black, display works! ===");
}

void loop() {
  delay(10000);
  LOG.println("alive...");
}
