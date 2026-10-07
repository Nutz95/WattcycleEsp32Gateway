#include "Display/TtgoStatusDisplay.h"

#include "Display/DisplayPins.h"

#ifndef UNIT_TEST

#include <Arduino.h>

#include <cstdio>

namespace wattcycle::display {
namespace {

constexpr uint16_t kBg = 0x10A2;
constexpr uint16_t kCard = 0x18E3;
constexpr uint16_t kAccent = 0x07F4;
constexpr uint16_t kMuted = 0x8410;
constexpr uint16_t kDanger = 0xF800;
constexpr uint16_t kWarn = 0xFE60;
constexpr uint16_t kBalance = 0xFE60;
constexpr uint16_t kText = 0xEF7D;

const char* kPageTitles[kPageCount] = {"Overview", "Cells", "Temps",
                                       "Alerts",   "Gateway", "ESP"};

}  // namespace

void TtgoStatusDisplay::begin() {
  pinMode(kBacklightGpio, OUTPUT);
  tft_.init();
  tft_.setRotation(1);
  tft_.fillScreen(kBg);
  // Prefer 8-bit sprite: ~32 KB vs ~65 KB, much safer with NimBLE heap pressure.
  sprite_.setColorDepth(8);
  sprite_.createSprite(kDisplayWidth, kDisplayHeight);
  if (!sprite_.created()) {
    return;
  }
  sprite_.setTextDatum(TL_DATUM);
  ready_ = true;
  setBacklight(true);
}

void TtgoStatusDisplay::setBacklight(bool on) {
  backlightOn_ = on;
  digitalWrite(kBacklightGpio, on ? HIGH : LOW);
}

bool TtgoStatusDisplay::isBacklightOn() const {
  return backlightOn_;
}

uint8_t TtgoStatusDisplay::pageIndex() const {
  return pageIndex_;
}

void TtgoStatusDisplay::nextPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + 1) % kPageCount);
}

void TtgoStatusDisplay::previousPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + kPageCount - 1) % kPageCount);
}

void TtgoStatusDisplay::drawChrome(const char* title) {
  sprite_.fillSprite(kBg);
  sprite_.fillRoundRect(4, 4, 232, 20, 5, kCard);
  sprite_.setTextColor(kAccent, kCard);
  sprite_.setTextSize(1);
  sprite_.drawString(title, 10, 9);
  const int dotsX = 168;
  for (uint8_t i = 0; i < kPageCount; ++i) {
    const int x = dotsX + static_cast<int>(i) * 10;
    if (i == pageIndex_) {
      sprite_.fillCircle(x, 14, 3, kAccent);
    } else {
      sprite_.fillCircle(x, 14, 2, kMuted);
    }
  }
}

void TtgoStatusDisplay::drawMetric(int x, int y, const char* label, const char* value) {
  sprite_.fillRoundRect(x, y, 110, 36, 6, kCard);
  sprite_.setTextColor(kMuted, kCard);
  sprite_.setTextSize(1);
  sprite_.drawString(label, x + 8, y + 6);
  sprite_.setTextColor(kText, kCard);
  sprite_.drawString(value, x + 8, y + 20);
}

void TtgoStatusDisplay::drawOverview(const telemetry::ITelemetryStore& store) {
  drawChrome(kPageTitles[0]);
  const auto battery = store.battery();
  const auto gateway = store.status();
  char line[32];

  if (!battery.valid) {
    sprite_.setTextColor(kWarn, kBg);
    sprite_.drawString("Waiting for BMS...", 12, 40);
    sprite_.setTextColor(kMuted, kBg);
    sprite_.drawString(gateway.lastError, 12, 58);
  } else {
    std::snprintf(line, sizeof(line), "%u%%", battery.stateOfChargePercent);
    sprite_.setTextColor(kAccent, kBg);
    sprite_.setTextSize(3);
    sprite_.drawString(line, 12, 30);
    sprite_.setTextSize(1);
    sprite_.drawRoundRect(12, 62, 160, 8, 3, kMuted);
    const int barW = static_cast<int>((160 * battery.stateOfChargePercent) / 100);
    if (barW > 0) {
      sprite_.fillRoundRect(12, 62, barW, 8, 3, kAccent);
    }
    std::snprintf(line, sizeof(line), "%.1f V", battery.moduleVoltage);
    drawMetric(8, 82, "Voltage", line);
    std::snprintf(line, sizeof(line), "%.1f A", battery.currentAmps);
    drawMetric(122, 82, "Current", line);
  }
  sprite_.setTextColor(gateway.wifiConnected ? kAccent : kDanger, kBg);
  sprite_.drawString(gateway.wifiConnected ? "WiFi" : "WiFi!", 190, 36);
  sprite_.setTextColor(gateway.bleConnected ? kAccent : kDanger, kBg);
  sprite_.drawString(gateway.bleConnected ? "BLE" : "BLE!", 190, 52);
  if (gateway.wifiConnected && gateway.wifiIp[0] != '\0' && gateway.webPort != 0) {
    std::snprintf(line, sizeof(line), ":%u", gateway.webPort);
    sprite_.setTextColor(kMuted, kBg);
    sprite_.drawString(line, 190, 68);
  }
}

void TtgoStatusDisplay::drawCells(const telemetry::ITelemetryStore& store) {
  drawChrome(kPageTitles[1]);
  const auto battery = store.battery();
  const auto warnings = store.warnings();
  if (!battery.valid || battery.cellCount == 0) {
    sprite_.setTextColor(kMuted, kBg);
    sprite_.drawString("No cell data", 12, 40);
    return;
  }

  float minV = battery.cellVoltages[0];
  float maxV = battery.cellVoltages[0];
  float sum = 0;
  for (uint8_t i = 0; i < battery.cellCount; ++i) {
    const float v = battery.cellVoltages[i];
    sum += v;
    if (v < minV) {
      minV = v;
    }
    if (v > maxV) {
      maxV = v;
    }
  }
  char line[40];
  std::snprintf(line, sizeof(line), "dV %.0fmV  bal %.2fA", (maxV - minV) * 1000.0f,
                battery.balanceCurrentAmps);
  sprite_.setTextColor(kMuted, kBg);
  sprite_.drawString(line, 10, 28);

  const uint8_t cols = battery.cellCount > 8 ? 8 : 4;
  const int cellW = cols == 8 ? 28 : 54;
  const int cellH = cols == 8 ? 40 : 36;
  const int gap = cols == 8 ? 1 : 4;
  for (uint8_t i = 0; i < battery.cellCount && i < 16; ++i) {
    const int x = 4 + (i % cols) * (cellW + gap);
    const int y = 44 + (i / cols) * (cellH + 2);
    uint16_t color = kText;
    if (warnings.cellBalancing[i]) {
      color = kBalance;
    } else if (battery.cellVoltages[i] >= maxV - 0.0001f) {
      color = kAccent;
    } else if (battery.cellVoltages[i] <= minV + 0.0001f) {
      color = kDanger;
    }
    sprite_.fillRoundRect(x, y, cellW, cellH, 3, kCard);
    std::snprintf(line, sizeof(line), "%u", i + 1);
    sprite_.setTextColor(kMuted, kCard);
    sprite_.drawString(line, x + 3, y + 3);
    if (cols == 8) {
      std::snprintf(line, sizeof(line), "%.2f", battery.cellVoltages[i]);
    } else {
      std::snprintf(line, sizeof(line), "%.3f", battery.cellVoltages[i]);
    }
    sprite_.setTextColor(color, kCard);
    sprite_.drawString(line, x + 3, y + (cols == 8 ? 18 : 16));
  }
}

void TtgoStatusDisplay::drawTemps(const telemetry::ITelemetryStore& store) {
  drawChrome(kPageTitles[2]);
  const auto battery = store.battery();
  char line[24];
  std::snprintf(line, sizeof(line), "%.1f C", battery.mosTemperatureC);
  drawMetric(8, 36, "MOS", line);
  std::snprintf(line, sizeof(line), "%.1f C", battery.pcbTemperatureC);
  drawMetric(122, 36, "PCB", line);
  sprite_.setTextColor(kMuted, kBg);
  sprite_.drawString("Cell sensors on web Temps tab", 10, 90);
}

void TtgoStatusDisplay::drawAlerts(const telemetry::ITelemetryStore& store) {
  drawChrome(kPageTitles[3]);
  const auto warnings = store.warnings();
  sprite_.setTextColor(warnings.hasActiveProtection ? kDanger : kAccent, kBg);
  sprite_.drawString(warnings.hasActiveProtection ? "Protection ACTIVE" : "Protection clear",
                     12, 40);
  sprite_.setTextColor(warnings.hasFault ? kDanger : kAccent, kBg);
  sprite_.drawString(warnings.hasFault ? "Fault ACTIVE" : "Fault clear", 12, 60);
  char line[40];
  std::snprintf(line, sizeof(line), "SR1 %02X  SR2 %02X  SR5 %02X", warnings.statusRegister1,
                warnings.statusRegister2, warnings.statusRegister5);
  sprite_.setTextColor(kMuted, kBg);
  sprite_.drawString(line, 12, 90);
}

void TtgoStatusDisplay::drawGateway(const telemetry::ITelemetryStore& store) {
  drawChrome(kPageTitles[4]);
  const auto gateway = store.status();
  const auto product = store.product();
  char line[40];
  if (gateway.webPort != 0) {
    std::snprintf(line, sizeof(line), "%s:%u", gateway.wifiIp, gateway.webPort);
  } else {
    std::snprintf(line, sizeof(line), "%s", gateway.wifiIp);
  }
  sprite_.setTextColor(kText, kBg);
  sprite_.drawString(line, 12, 36);
  sprite_.drawString(gateway.bleAddress, 12, 54);
  sprite_.setTextColor(kMuted, kBg);
  sprite_.drawString(product.manufacturerName, 12, 78);
  sprite_.drawString(product.serialNumber, 12, 96);
}

void TtgoStatusDisplay::drawEsp(const telemetry::ITelemetryStore& store) {
  drawChrome(kPageTitles[5]);
  const auto esp = store.espHealth();
  char line[24];
  std::snprintf(line, sizeof(line), "%u%%", esp.cpuCore0Percent);
  drawMetric(8, 36, "CPU0", line);
  std::snprintf(line, sizeof(line), "%u%%", esp.cpuCore1Percent);
  drawMetric(122, 36, "CPU1", line);
  std::snprintf(line, sizeof(line), "%.1f C", esp.chipTemperatureC);
  drawMetric(8, 84, "Chip", line);
  std::snprintf(line, sizeof(line), "%u KB", esp.freeHeapBytes / 1024u);
  drawMetric(122, 84, "Heap", line);
}

void TtgoStatusDisplay::render(const telemetry::ITelemetryStore& store) {
  if (!ready_) {
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
      drawGateway(store);
      break;
    case 5:
      drawEsp(store);
      break;
    default:
      drawOverview(store);
      break;
  }
  sprite_.pushSprite(0, 0);
}

void TtgoStatusDisplay::renderAuthPrompt(const auth::AuthPrompt& prompt) {
  if (!ready_ || prompt.kind == auth::AuthPromptKind::None) {
    return;
  }

  sprite_.fillSprite(kBg);
  sprite_.fillRoundRect(4, 4, 232, 20, 5, kCard);
  sprite_.setTextColor(kAccent, kCard);
  sprite_.setTextSize(1);
  if (prompt.kind == auth::AuthPromptKind::ConfirmSetup) {
    sprite_.drawString("Confirm web login", 10, 9);
    sprite_.setTextColor(kText, kBg);
    sprite_.drawString("User:", 12, 36);
    sprite_.setTextColor(kAccent, kBg);
    sprite_.drawString(prompt.username, 52, 36);
    sprite_.setTextColor(kMuted, kBg);
    sprite_.drawString("Press TOP to save to NVS", 12, 54);
  } else {
    sprite_.drawString("Reset web password", 10, 9);
    sprite_.setTextColor(kWarn, kBg);
    sprite_.drawString("Clear stored credentials?", 12, 40);
    sprite_.setTextColor(kMuted, kBg);
    sprite_.drawString("Then create a new login", 12, 56);
  }

  // Button hints: GPIO35 top / GPIO0 bottom on the right bezel.
  sprite_.fillRoundRect(150, 78, 82, 22, 4, kCard);
  sprite_.setTextColor(kAccent, kCard);
  sprite_.drawString("TOP: OK", 158, 84);
  sprite_.fillRoundRect(150, 106, 82, 22, 4, kCard);
  sprite_.setTextColor(kDanger, kCard);
  sprite_.drawString("BOT: Cancel", 154, 112);
  sprite_.pushSprite(0, 0);
}

}  // namespace wattcycle::display

#endif
