/**
 * Shelf — BLE E-Ink Dashboard for Xteink X4
 *
 * Receives display framebuffer data over BLE GATT.
 * A phone/Mac renders the dashboard image, dithers to 1-bit,
 * and pushes 48000 bytes over BLE in chunks.
 *
 * BLE Protocol:
 *   Service:  4153484C-4600-1000-8000-00805F9B34FB  ("SHELF")
 *   Control:  4153484C-4601-... — write command bytes
 *     0x01 = start transfer (resets buffer offset to 0)
 *     0x02 = render (display current buffer, FAST_REFRESH)
 *     0x03 = render full (FULL_REFRESH)
 *     0x04 = clear screen (white)
 *   Data:     4153484C-4602-... — write framebuffer chunks (up to 512B each)
 *   Status:   4153484C-4603-... — read: bytes received (uint32_t LE)
 *
 * Test with LightBlue app:
 *   1. Connect to "Shelf"
 *   2. Write 0x01 to Control characteristic (start transfer)
 *   3. Write chunks to Data characteristic
 *   4. Write 0x02 to Control characteristic (render)
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <EInkDisplay.h>
#include "config.h"

#define LOG Serial

// ─── Display ───
EInkDisplay display(EPD_SCLK, EPD_MOSI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

// ─── BLE UUIDs ───
#define SERVICE_UUID        "4153484C-4600-1000-8000-00805F9B34FB"
#define CHAR_CONTROL_UUID   "4153484C-4601-1000-8000-00805F9B34FB"
#define CHAR_DATA_UUID      "4153484C-4602-1000-8000-00805F9B34FB"
#define CHAR_STATUS_UUID    "4153484C-4603-1000-8000-00805F9B34FB"

// ─── State ───
static uint32_t bufferOffset = 0;
static bool renderRequested = false;
static bool fullRefreshRequested = false;
static bool clearRequested = false;
static bool connected = false;

NimBLECharacteristic* statusChar = nullptr;

// Update the status characteristic with bytes received
void updateStatus() {
  if (statusChar) {
    uint8_t data[4];
    data[0] = bufferOffset & 0xFF;
    data[1] = (bufferOffset >> 8) & 0xFF;
    data[2] = (bufferOffset >> 16) & 0xFF;
    data[3] = (bufferOffset >> 24) & 0xFF;
    statusChar->setValue(data, 4);
    statusChar->notify();
  }
}

// ─── Control Characteristic Callback ───
class ControlCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    const uint8_t* data = pChar->getValue().data();
    size_t len = pChar->getValue().length();
    if (len < 1) return;

    switch (data[0]) {
      case 0x01: // Start transfer
        bufferOffset = 0;
        LOG.println("BLE: Transfer started, offset reset");
        updateStatus();
        break;
      case 0x02: // Render (fast)
        renderRequested = true;
        LOG.printf("BLE: Render requested (%lu bytes received)\n", bufferOffset);
        break;
      case 0x03: // Render (full refresh)
        fullRefreshRequested = true;
        LOG.printf("BLE: Full render requested (%lu bytes received)\n", bufferOffset);
        break;
      case 0x04: // Clear
        clearRequested = true;
        LOG.println("BLE: Clear requested");
        break;
      default:
        LOG.printf("BLE: Unknown command 0x%02X\n", data[0]);
    }
  }
};

// ─── Data Characteristic Callback ───
class DataCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pChar, NimBLEConnInfo& connInfo) override {
    const uint8_t* data = pChar->getValue().data();
    size_t len = pChar->getValue().length();

    uint8_t* fb = display.getFrameBuffer();
    uint32_t maxSize = EInkDisplay::BUFFER_SIZE; // 48000

    if (bufferOffset + len > maxSize) {
      len = maxSize - bufferOffset; // Clamp
    }

    if (len > 0) {
      memcpy(fb + bufferOffset, data, len);
      bufferOffset += len;
    }

    // Log progress every ~10%
    if (bufferOffset % 4800 < len) {
      LOG.printf("BLE: %lu / %lu bytes (%lu%%)\n", bufferOffset, maxSize, (bufferOffset * 100) / maxSize);
      updateStatus();
    }
  }
};

// ─── Connection Callbacks ───
class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    connected = true;
    LOG.println("BLE: Client connected");
    // Allow higher MTU for faster transfers
    // NimBLE handles MTU negotiation automatically
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    connected = false;
    LOG.println("BLE: Client disconnected");
    // Restart advertising
    NimBLEDevice::startAdvertising();
  }
};

// ─── Setup ───
void setup() {
  LOG.begin(115200);
  delay(2000);

  LOG.println();
  LOG.println("================================");
  LOG.println("  Shelf BLE — E-Ink Dashboard");
  LOG.println("  Xteink X4 · BLE GATT Server");
  LOG.println("================================");

  // Init display
  LOG.println("Initializing display...");
  display.begin();
  LOG.println("Display OK");

  // Clear to white on boot
  display.clearScreen(0xFF);
  display.displayBuffer(EInkDisplay::FULL_REFRESH, false);
  LOG.println("Screen cleared to white");

  // Init BLE
  LOG.println("Starting BLE...");
  NimBLEDevice::init("Shelf");
  NimBLEDevice::setMTU(517); // Request large MTU for faster transfers

  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  // Control characteristic (write)
  NimBLECharacteristic* controlChar = pService->createCharacteristic(
    CHAR_CONTROL_UUID,
    NIMBLE_PROPERTY::WRITE
  );
  controlChar->setCallbacks(new ControlCallbacks());

  // Data characteristic (write, no response for speed)
  NimBLECharacteristic* dataChar = pService->createCharacteristic(
    CHAR_DATA_UUID,
    NIMBLE_PROPERTY::WRITE_NR  // Write without response = faster
  );
  dataChar->setCallbacks(new DataCallbacks());

  // Status characteristic (read + notify)
  statusChar = pService->createCharacteristic(
    CHAR_STATUS_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
  );
  uint32_t zero = 0;
  statusChar->setValue((uint8_t*)&zero, 4);

  

  // Start advertising
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  
  pAdvertising->start();

  LOG.println("BLE advertising as 'Shelf'");
  LOG.println("Ready! Connect with LightBlue or Daslab.");
  LOG.printf("Framebuffer size: %lu bytes\n", (uint32_t)EInkDisplay::BUFFER_SIZE);
}

// ─── Loop ───
void loop() {
  if (clearRequested) {
    clearRequested = false;
    LOG.println("Clearing screen...");
    display.clearScreen(0xFF);
    display.displayBuffer(EInkDisplay::FULL_REFRESH, true);
    LOG.println("Screen cleared");
  }

  if (renderRequested) {
    renderRequested = false;
    LOG.println("Rendering (fast refresh)...");
    display.displayBuffer(EInkDisplay::FAST_REFRESH, true);
    LOG.println("Render complete!");
  }

  if (fullRefreshRequested) {
    fullRefreshRequested = false;
    LOG.println("Rendering (full refresh)...");
    display.displayBuffer(EInkDisplay::FULL_REFRESH, true);
    LOG.println("Full render complete!");
  }

  delay(50);
}
