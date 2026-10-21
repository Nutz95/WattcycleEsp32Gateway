#include "Display/DongleStatusDisplay.h"

#include "boards/dongle_s3/DongleEspHealthLines.h"
#include "boards/dongle_s3/DonglePanelPalette.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstdio>

namespace wattcycle::display {
namespace {

using boards::dongle_s3::DonglePanelPalette;

const char* kPageTitles[6] = {"BMS Overview", "BMS Cells", "BMS Temps", "BMS Alerts", "BMS Link", "BMS ESP"};

void textLine(TFT_eSprite& canvas, int y, const char* text, uint16_t color) {
  canvas.setTextColor(color, DonglePanelPalette::kBackground);
  canvas.setTextSize(1);
  canvas.drawString(text, 4, y);
}

}  // namespace

void DongleStatusDisplay::begin() {
  panel_.begin("bms-ui", "WATTCYCLE");
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
  char line[40];

  if (!battery.valid) {
    textLine(canvas, 20, "Waiting BMS...", DonglePanelPalette::kWarn);
    textLine(canvas, 34, gateway.lastError[0] ? gateway.lastError : "no frame yet",
             DonglePanelPalette::kMuted);
  } else {
    std::snprintf(line, sizeof(line), "%u%%  %.1fV", battery.stateOfChargePercent,
                  battery.moduleVoltage);
    textLine(canvas, 18, line, DonglePanelPalette::kAccent);
    std::snprintf(line, sizeof(line), "%.1fA  %.0fW", battery.currentAmps, battery.powerWatts);
    textLine(canvas, 32, line, DonglePanelPalette::kText);
    std::snprintf(line, sizeof(line), "%.1f/%.1fAh", battery.remainingCapacityAh,
                  battery.totalCapacityAh);
    textLine(canvas, 46, line, DonglePanelPalette::kMuted);
  }
  std::snprintf(line, sizeof(line), "%s  %s  ch%u", gateway.bleConnected ? "BLE" : "BLE!",
                gateway.espNowReady ? "NOW" : "NOW!", gateway.espNowChannel);
  textLine(canvas, 62, line,
           (gateway.bleConnected && gateway.espNowReady) ? DonglePanelPalette::kAccent
                                                         : DonglePanelPalette::kDanger);
}

void DongleStatusDisplay::drawCells(const telemetry::ITelemetryStore& store) {
  panel_.drawChrome(kPageTitles[1], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto battery = store.battery();
  if (!battery.valid || battery.cellCount == 0) {
    textLine(canvas, 28, "No cell data", DonglePanelPalette::kMuted);
    return;
  }
  float minV = battery.cellVoltages[0];
  float maxV = minV;
  for (uint8_t i = 1; i < battery.cellCount; ++i) {
    if (battery.cellVoltages[i] < minV) minV = battery.cellVoltages[i];
    if (battery.cellVoltages[i] > maxV) maxV = battery.cellVoltages[i];
  }
  const auto warnings = store.warnings();
  char line[40];
  std::snprintf(line, sizeof(line), "dV %.0fmV  n=%u", (maxV - minV) * 1000.0f, battery.cellCount);
  textLine(canvas, 18, line, DonglePanelPalette::kMuted);
  const uint8_t shown = battery.cellCount > 8 ? 8 : battery.cellCount;
  for (uint8_t i = 0; i < shown; ++i) {
    const int col = (i < 4) ? 0 : 1;
    const int row = static_cast<int>(i % 4);
    const float voltage = battery.cellVoltages[i];
    uint16_t color = DonglePanelPalette::kText;
    if (warnings.cellBalancing[i]) {
      color = DonglePanelPalette::kWarn;
    } else if (voltage >= maxV - 0.0001f) {
      color = DonglePanelPalette::kAccent;
    } else if (voltage <= minV + 0.0001f) {
      color = DonglePanelPalette::kDanger;
    }
    std::snprintf(line, sizeof(line), "%u %.3f", i + 1, voltage);
    canvas.setTextColor(color, DonglePanelPalette::kBackground);
    canvas.setTextSize(1);
    canvas.drawString(line, 4 + col * 78, 30 + row * 12);
  }
}

void DongleStatusDisplay::drawTemps(const telemetry::ITelemetryStore& store) {
  panel_.drawChrome(kPageTitles[2], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto battery = store.battery();
  char line[32];
  std::snprintf(line, sizeof(line), "MOS  %.1f C", battery.mosTemperatureC);
  textLine(canvas, 24, line, DonglePanelPalette::kText);
  std::snprintf(line, sizeof(line), "PCB  %.1f C", battery.pcbTemperatureC);
  textLine(canvas, 40, line, DonglePanelPalette::kText);
  textLine(canvas, 58, "Cell temps: web Temps", DonglePanelPalette::kMuted);
}

void DongleStatusDisplay::drawAlerts(const telemetry::ITelemetryStore& store) {
  panel_.drawChrome(kPageTitles[3], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto warnings = store.warnings();
  textLine(canvas, 20, warnings.hasActiveProtection ? "Protection ACTIVE" : "Protection clear",
           warnings.hasActiveProtection ? DonglePanelPalette::kDanger : DonglePanelPalette::kAccent);
  textLine(canvas, 34, warnings.hasFault ? "Fault ACTIVE" : "Fault clear",
           warnings.hasFault ? DonglePanelPalette::kDanger : DonglePanelPalette::kAccent);
  char line[40];
  std::snprintf(line, sizeof(line), "SR %02X %02X %02X", warnings.statusRegister1,
                warnings.statusRegister2, warnings.statusRegister5);
  textLine(canvas, 52, line, DonglePanelPalette::kMuted);
}

void DongleStatusDisplay::drawLink(const telemetry::ITelemetryStore& store) {
  panel_.drawChrome(kPageTitles[4], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto gateway = store.status();
  const auto product = store.product();
  char line[48];
  std::snprintf(line, sizeof(line), "NOW %s ch%u", gateway.espNowReady ? "OK" : "DOWN",
                gateway.espNowChannel);
  textLine(canvas, 18, line,
           gateway.espNowReady ? DonglePanelPalette::kAccent : DonglePanelPalette::kDanger);
  textLine(canvas, 30, gateway.espNowPeerMac[0] ? gateway.espNowPeerMac : "(no peer)",
           DonglePanelPalette::kText);
  textLine(canvas, 42, gateway.bleAddress[0] ? gateway.bleAddress : "(no BLE mac)",
           DonglePanelPalette::kText);
  const char* tail = gateway.lastError[0] ? gateway.lastError
                     : (product.serialNumber[0] ? product.serialNumber : product.manufacturerName);
  textLine(canvas, 56, tail[0] ? tail : "link idle", DonglePanelPalette::kMuted);
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
  panel_.drawEspPage(kPageTitles[5], pageIndex_, kPageCount, lines);
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
      drawCells(store);
      break;
    case 2:
      drawTemps(store);
      break;
    case 3:
      drawAlerts(store);
      break;
    case 4:
      drawLink(store);
      break;
    case 5:
      drawEsp(store);
      break;
    default:
      drawOverview(store);
      break;
  }
  panel_.present();
}

}  // namespace wattcycle::display
