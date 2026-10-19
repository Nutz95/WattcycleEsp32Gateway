#include "Display/DongleStatusDisplay.h"

#include "boards/dongle_s3/DongleEspHealthLines.h"
#include "boards/dongle_s3/DonglePanelPalette.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstdio>

namespace wattcycle::display {
namespace {

using boards::dongle_s3::DonglePanelPalette;

const char* kPageTitles[4] = {"BMS Overview", "BMS Energy", "BMS Link", "BMS ESP"};

}  // namespace

void DongleStatusDisplay::begin() {
  panel_.begin("bms-ui", "BMS bridge");
}

void DongleStatusDisplay::setBacklight(bool on) {
  panel_.setBacklight(on);
}

bool DongleStatusDisplay::isBacklightOn() const {
  return panel_.isBacklightOn();
}

uint8_t DongleStatusDisplay::pageIndex() const {
  return pageIndex_;
}

void DongleStatusDisplay::nextPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + 1) % kPageCount);
}

void DongleStatusDisplay::previousPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + kPageCount - 1) % kPageCount);
}

void DongleStatusDisplay::drawOverview(const telemetry::ITelemetryStore& store) {
  panel_.drawChrome(kPageTitles[0], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto battery = store.battery();
  const auto gateway = store.status();
  char line[28];

  if (!battery.valid) {
    canvas.setTextColor(DonglePanelPalette::kWarn, DonglePanelPalette::kBackground);
    canvas.drawString("Waiting BMS...", 6, 28);
    canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
    canvas.drawString(gateway.lastError, 6, 44);
    return;
  }
  std::snprintf(line, sizeof(line), "%u%%", battery.stateOfChargePercent);
  canvas.setTextColor(DonglePanelPalette::kAccent, DonglePanelPalette::kBackground);
  canvas.setTextSize(2);
  canvas.drawString(line, 6, 22);
  canvas.setTextSize(1);
  std::snprintf(line, sizeof(line), "%.1fV  %.1fA", battery.moduleVoltage, battery.currentAmps);
  canvas.setTextColor(DonglePanelPalette::kText, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 48);
  canvas.setTextColor(gateway.bleConnected ? DonglePanelPalette::kAccent : DonglePanelPalette::kDanger,
                      DonglePanelPalette::kBackground);
  canvas.drawString(gateway.bleConnected ? "BLE" : "BLE!", 110, 28);
  canvas.setTextColor(gateway.espNowReady ? DonglePanelPalette::kAccent : DonglePanelPalette::kDanger,
                      DonglePanelPalette::kBackground);
  canvas.drawString(gateway.espNowReady ? "NOW" : "NOW!", 110, 44);
}

void DongleStatusDisplay::drawEnergy(const telemetry::ITelemetryStore& store) {
  panel_.drawChrome(kPageTitles[1], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto battery = store.battery();
  char line[28];
  if (!battery.valid) {
    canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
    canvas.drawString("No battery data", 6, 32);
    return;
  }
  std::snprintf(line, sizeof(line), "MOS %.0fC", battery.mosTemperatureC);
  canvas.setTextColor(DonglePanelPalette::kText, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 24);
  std::snprintf(line, sizeof(line), "PCB %.0fC", battery.pcbTemperatureC);
  canvas.drawString(line, 6, 40);
  std::snprintf(line, sizeof(line), "cells %u", battery.cellCount);
  canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 56);
}

void DongleStatusDisplay::drawLink(const telemetry::ITelemetryStore& store) {
  panel_.drawChrome(kPageTitles[2], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto gateway = store.status();
  char line[36];
  canvas.setTextColor(gateway.bleConnected ? DonglePanelPalette::kAccent : DonglePanelPalette::kDanger,
                      DonglePanelPalette::kBackground);
  canvas.drawString(gateway.bleConnected ? "BLE up" : "BLE down", 6, 20);
  if (gateway.bleConnected) {
    std::snprintf(line, sizeof(line), "tries %lu",
                  static_cast<unsigned long>(gateway.bleConnectAttempts));
    canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
    canvas.drawString(line, 70, 20);
  } else {
    const uint32_t nowMs = millis();
    if (gateway.bleNextRetryAtMs > nowMs) {
      const uint32_t secondsLeft = (gateway.bleNextRetryAtMs - nowMs + 999u) / 1000u;
      std::snprintf(line, sizeof(line), "try#%lu next %lus",
                    static_cast<unsigned long>(gateway.bleConnectAttempts),
                    static_cast<unsigned long>(secondsLeft));
    } else if (gateway.bleConnectAttempts > 0) {
      std::snprintf(line, sizeof(line), "try#%lu connecting",
                    static_cast<unsigned long>(gateway.bleConnectAttempts));
    } else {
      std::snprintf(line, sizeof(line), "waiting BLE...");
    }
    canvas.setTextColor(DonglePanelPalette::kWarn, DonglePanelPalette::kBackground);
    canvas.drawString(line, 6, 34);
  }
  std::snprintf(line, sizeof(line), "NOW %s ch%u", gateway.espNowReady ? "OK" : "--",
                gateway.espNowChannel);
  canvas.setTextColor(DonglePanelPalette::kText, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, gateway.bleConnected ? 38 : 50);
  if (gateway.bleConnected) {
    canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
    canvas.drawString(gateway.espNowPeerMac[0] ? gateway.espNowPeerMac : "(no peer)", 6, 54);
  }
}

void DongleStatusDisplay::drawEsp(const telemetry::ITelemetryStore& store) {
  const auto espHealth = store.espHealth();
  const String mac = WiFi.macAddress();
  boards::dongle_s3::DongleEspHealthLines lines;
  lines.macAddress = mac.length() ? mac.c_str() : "";
  lines.cpuCore0Percent = espHealth.cpuCore0Percent;
  lines.cpuCore1Percent = espHealth.cpuCore1Percent;
  lines.freeHeapBytes = espHealth.freeHeapBytes;
  lines.uptimeSeconds = espHealth.uptimeSeconds;
  lines.chipTemperatureC = espHealth.chipTemperatureC;
  panel_.drawEspPage(kPageTitles[3], pageIndex_, kPageCount, lines);
}

void DongleStatusDisplay::render(const telemetry::ITelemetryStore& store) {
  if (!panel_.ready()) {
    return;
  }
  switch (pageIndex_) {
    case 0:
      drawOverview(store);
      break;
    case 1:
      drawEnergy(store);
      break;
    case 2:
      drawLink(store);
      break;
    case 3:
      drawEsp(store);
      break;
    default:
      drawOverview(store);
      break;
  }
  panel_.present();
}

}  // namespace wattcycle::display
