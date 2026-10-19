#include "Display/DongleStatusDisplay.h"

#include "boards/dongle_s3/DongleEspHealthLines.h"
#include "boards/dongle_s3/DonglePanelPalette.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstdio>

namespace ecoflow::display {
namespace {

using boards::dongle_s3::DonglePanelPalette;

const char* kPageTitles[4] = {"EF Overview", "EF Ports", "EF Link", "EF ESP"};

}  // namespace

void DongleStatusDisplay::begin() {
  panel_.begin("ecoflow-ui", "EcoFlow");
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

void DongleStatusDisplay::drawOverview(const ble::EcoFlowBleSnapshot& bleSnapshot) {
  panel_.drawChrome(kPageTitles[0], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  char line[32];
  if (!bleSnapshot.telemetry.valid) {
    canvas.setTextColor(DonglePanelPalette::kWarn, DonglePanelPalette::kBackground);
    canvas.drawString(bleSnapshot.connected ? "Waiting data..." : "BLE down", 6, 28);
    canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
    canvas.drawString(bleSnapshot.lastError, 6, 44);
    return;
  }
  std::snprintf(line, sizeof(line), "%u%%", bleSnapshot.telemetry.socPercent);
  canvas.setTextColor(DonglePanelPalette::kAccent, DonglePanelPalette::kBackground);
  canvas.setTextSize(2);
  canvas.drawString(line, 6, 20);
  canvas.setTextSize(1);
  if (bleSnapshot.telemetry.remainMinutes > 0) {
    std::snprintf(line, sizeof(line), "%umin", bleSnapshot.telemetry.remainMinutes);
    canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
    canvas.drawString(line, 70, 28);
  }
  std::snprintf(line, sizeof(line), "ACout %dW  PV %dW", bleSnapshot.telemetry.acOutputW,
                bleSnapshot.telemetry.solarInputW);
  canvas.setTextColor(DonglePanelPalette::kText, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 52);
}

void DongleStatusDisplay::drawPorts(const ble::EcoFlowBleSnapshot& bleSnapshot) {
  panel_.drawChrome(kPageTitles[1], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  char line[36];
  const auto& telemetry = bleSnapshot.telemetry;
  std::snprintf(line, sizeof(line), "AC %s %dW", telemetry.acOutputOn ? "ON" : "off",
                telemetry.acOutputW);
  canvas.setTextColor(telemetry.acOutputOn ? DonglePanelPalette::kAccent : DonglePanelPalette::kMuted,
                      DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 22);
  std::snprintf(line, sizeof(line), "DC %s %dW", telemetry.dcOutputOn ? "ON" : "off",
                telemetry.dcOutputW);
  canvas.setTextColor(telemetry.dcOutputOn ? DonglePanelPalette::kAccent : DonglePanelPalette::kMuted,
                      DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 38);
  std::snprintf(line, sizeof(line), "USB %s %dW", telemetry.usbOutputOn ? "ON" : "off",
                telemetry.usbOutputW);
  canvas.setTextColor(telemetry.usbOutputOn ? DonglePanelPalette::kAccent : DonglePanelPalette::kMuted,
                      DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 54);
  if (telemetry.haveTemperature) {
    std::snprintf(line, sizeof(line), "%dC", static_cast<int>(telemetry.tempC));
    canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
    canvas.drawString(line, 120, 54);
  }
}

void DongleStatusDisplay::drawLink(const ble::EcoFlowBleSnapshot& bleSnapshot) {
  panel_.drawChrome(kPageTitles[2], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  char line[36];
  canvas.setTextColor(bleSnapshot.connected ? DonglePanelPalette::kAccent : DonglePanelPalette::kDanger,
                      DonglePanelPalette::kBackground);
  canvas.drawString(bleSnapshot.connected ? "BLE up" : "BLE down", 6, 22);
  canvas.setTextColor(
      bleSnapshot.authenticated ? DonglePanelPalette::kAccent : DonglePanelPalette::kWarn,
      DonglePanelPalette::kBackground);
  canvas.drawString(bleSnapshot.authenticated ? "auth OK" : "auth --", 70, 22);
  std::snprintf(line, sizeof(line), "tel %lu  ntf %lu",
                static_cast<unsigned long>(bleSnapshot.telemetryCount),
                static_cast<unsigned long>(bleSnapshot.notifyCount));
  canvas.setTextColor(DonglePanelPalette::kMuted, DonglePanelPalette::kBackground);
  canvas.drawString(line, 6, 40);
  canvas.setTextColor(DonglePanelPalette::kText, DonglePanelPalette::kBackground);
  canvas.drawString(bleSnapshot.lastError[0] ? bleSnapshot.lastError : "ok", 6, 56);
}

void DongleStatusDisplay::drawEsp(const esp_sys::EspHealthSnapshot& health) {
  const String mac = WiFi.macAddress();
  boards::dongle_s3::DongleEspHealthLines lines;
  lines.macAddress = mac.length() ? mac.c_str() : "";
  lines.cpuCore0Percent = health.cpuCore0Percent;
  lines.cpuCore1Percent = health.cpuCore1Percent;
  lines.freeHeapBytes = health.freeHeapBytes;
  lines.uptimeSeconds = health.uptimeSeconds;
  lines.chipTemperatureC = health.chipTemperatureC;
  panel_.drawEspPage(kPageTitles[3], pageIndex_, kPageCount, lines);
}

void DongleStatusDisplay::render(const ble::EcoFlowBleSnapshot& bleSnapshot,
                                 const esp_sys::EspHealthSnapshot& health) {
  if (!panel_.ready()) {
    return;
  }
  switch (pageIndex_) {
    case 0:
      drawOverview(bleSnapshot);
      break;
    case 1:
      drawPorts(bleSnapshot);
      break;
    case 2:
      drawLink(bleSnapshot);
      break;
    case 3:
      drawEsp(health);
      break;
    default:
      drawOverview(bleSnapshot);
      break;
  }
  panel_.present();
}

}  // namespace ecoflow::display
