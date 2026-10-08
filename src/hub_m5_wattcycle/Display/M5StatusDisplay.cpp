#include "Display/M5StatusDisplay.h"

#include "Bus/SharedSpiLock.h"
#include "Display/DisplayPins.h"

#include <cstdio>
#include <cstring>

namespace wattcycle::display {
namespace {
#ifndef UNIT_TEST
// Metro tiles: white text always; tile fill encodes status / sign.
// Canvas is 8-bit (RGB332) — vivid primaries survive quantization.
constexpr uint16_t kColorBg = 0x08C4;
constexpr uint16_t kColorTileA = 0x03BF;     // Win blue
constexpr uint16_t kColorTileB = 0x0451;     // teal
constexpr uint16_t kColorTileC = 0xEAA0;     // orange
constexpr uint16_t kColorTileD = 0x901A;     // magenta
constexpr uint16_t kColorFooterTile = 0x0257;
constexpr uint16_t kColorOk = 0x05E0;        // green tile (charge / OK)
constexpr uint16_t kColorBad = 0xF800;       // red tile (discharge / down)
constexpr uint16_t kColorWarn = 0xEAA0;      // amber tile (pending / stale)
constexpr int kTileRadius = 2;

const char* kPageNames[kPageCount] = {"Overview", "Pack", "Solar",
                                      "Gateway", "ESP", "Temps"};

uint16_t statusTileFill(bool ok) {
  return ok ? kColorOk : kColorBad;
}

uint16_t warnTileFill(bool ok) {
  return ok ? kColorOk : kColorWarn;
}

uint16_t signedTileFill(float value, uint16_t idleFill) {
  if (value > 0.05f) {
    return kColorOk;
  }
  if (value < -0.05f) {
    return kColorBad;
  }
  return idleFill;
}

void drawTile(M5Canvas& canvas, int x, int y, int w, int h, uint16_t fill,
              const char* title, const char* value, uint8_t valueSize = 2) {
  canvas.fillRoundRect(x, y, w, h, kTileRadius, fill);
  canvas.setTextColor(TFT_WHITE, fill);
  canvas.setTextSize(1);
  canvas.setCursor(x + 8, y + 8);
  canvas.print(title != nullptr ? title : "");
  canvas.setTextSize(valueSize);
  canvas.setCursor(x + 8, y + (valueSize >= 2 ? 28 : 30));
  canvas.print(value != nullptr ? value : "--");
}

void drawSplashFrame(M5Canvas& canvas) {
  canvas.fillSprite(kColorBg);
  canvas.fillRoundRect(16, 40, 288, 100, 4, kColorTileA);
  canvas.fillRoundRect(16, 156, 288, 40, 4, kColorTileB);
  canvas.setTextColor(TFT_WHITE, kColorTileA);
  canvas.setTextSize(2);
  canvas.setCursor(36, 68);
  canvas.print("WattCycle");
  canvas.setTextSize(1);
  canvas.setCursor(36, 100);
  canvas.print("M5 hub");
  canvas.setTextColor(TFT_WHITE, kColorTileB);
  canvas.setCursor(36, 170);
  canvas.print("Starting...");
}

void drawFooterTile(M5Canvas& canvas, int x, int y, int w, int h, const char* top,
                    const char* bottom) {
  canvas.fillRoundRect(x, y, w, h, kTileRadius, kColorFooterTile);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, kColorFooterTile);
  canvas.setCursor(x + 8, y + 6);
  canvas.print(top != nullptr ? top : "");
  canvas.setCursor(x + 8, y + 20);
  canvas.print(bottom != nullptr ? bottom : "");
}
#endif
}  // namespace

void M5StatusDisplay::begin() {
#ifndef UNIT_TEST
  bus::SharedSpiLock::begin();
  auto boardConfiguration = M5.config();
  boardConfiguration.internal_imu = false;
  boardConfiguration.internal_rtc = false;
  boardConfiguration.internal_spk = false;
  boardConfiguration.internal_mic = false;
  M5.begin(boardConfiguration);
  M5.Display.setRotation(1);
  canvas_.setColorDepth(8);
  if (!canvas_.createSprite(kDisplayWidth, kDisplayHeight)) {
    Serial.printf("M5 canvas alloc failed heap=%u\n", ESP.getFreeHeap());
    ready_ = false;
    return;
  }
  canvas_.setTextSize(2);
  ready_ = true;
  setBacklight(true);
  drawSplashFrame(canvas_);
  pushFrame();
  Serial.printf("M5 display ready heap=%u\n", ESP.getFreeHeap());
#else
  ready_ = true;
#endif
}

void M5StatusDisplay::setBacklight(bool on) {
  backlightOn_ = on;
#ifndef UNIT_TEST
  M5.Display.setBrightness(on ? 80 : 0);
#endif
}

bool M5StatusDisplay::isBacklightOn() const {
  return backlightOn_;
}

uint8_t M5StatusDisplay::pageIndex() const {
  return pageIndex_;
}

void M5StatusDisplay::nextPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + 1) % kPageCount);
}

void M5StatusDisplay::previousPage() {
  pageIndex_ = static_cast<uint8_t>((pageIndex_ + kPageCount - 1) % kPageCount);
}

void M5StatusDisplay::pushFrame() {
#ifndef UNIT_TEST
  bus::SpiGuard guard;
  canvas_.pushSprite(0, 0);
#endif
}

void M5StatusDisplay::drawButtonFooter(const char* centerLabel) {
#ifndef UNIT_TEST
  const int y = kDisplayHeight - kFooterHeight + 2;
  const int h = kFooterHeight - 4;
  const int gap = 4;
  const int tileW = (kDisplayWidth - 16 - gap * 2) / 3;
  const bool authMode = centerLabel != nullptr && std::strcmp(centerLabel, "OK") == 0;
  const char* midTop =
      authMode ? "OK"
               : ((pageIndex_ < kPageCount) ? kPageNames[pageIndex_] : "Page");
  const char* midBottom = authMode ? "confirm" : "B";
  char nextLine[16] = {};
  std::snprintf(nextLine, sizeof(nextLine), "next %u/%u", static_cast<unsigned>(pageIndex_ + 1),
                static_cast<unsigned>(kPageCount));
  drawFooterTile(canvas_, 8, y, tileW, h, "A  <", "prev");
  drawFooterTile(canvas_, 8 + tileW + gap, y, tileW, h, midTop, midBottom);
  drawFooterTile(canvas_, 8 + (tileW + gap) * 2, y, tileW, h, ">  C", nextLine);
#else
  (void)centerLabel;
#endif
}

void M5StatusDisplay::drawOverview(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto battery = store.battery();
  const auto solar = store.solar();
  const auto gateway = store.status();
  canvas_.fillSprite(kColorBg);

  char soc[16] = "--%";
  if (battery.valid) {
    std::snprintf(soc, sizeof(soc), "%u%%", battery.stateOfChargePercent);
  }
  char solarPower[20] = "XT --";
  uint16_t solarFill = kColorTileB;
  if (solar.linkFresh) {
    std::snprintf(solarPower, sizeof(solarPower), "%.0fW", static_cast<double>(solar.powerW));
    solarFill = signedTileFill(solar.powerW, kColorTileB);
  }
  char wifi[16] = "DOWN";
  if (gateway.wifiConnected) {
    std::snprintf(wifi, sizeof(wifi), "OK");
  }
  char clockTitle[16] = "CLOCK";
  char clockValue[16] = "NTP wait";
  uint16_t clockFill = kColorWarn;
  if (gateway.ntpSynced && gateway.localTime[0] != '\0') {
    std::snprintf(clockTitle, sizeof(clockTitle), "%s", gateway.localDate);
    std::snprintf(clockValue, sizeof(clockValue), "%s", gateway.localTime);
    clockFill = kColorOk;
  }

  drawTile(canvas_, 8, 8, 148, 72, kColorTileA, "PACK SOC", soc);
  drawTile(canvas_, 164, 8, 148, 72, solarFill, "SOLAR", solarPower);
  drawTile(canvas_, 8, 88, 148, 72, statusTileFill(gateway.wifiConnected), "NETWORK", wifi);
  drawTile(canvas_, 164, 88, 148, 72, clockFill, clockTitle, clockValue);

  drawButtonFooter("Overview");
  pushFrame();
#else
  (void)store;
#endif
}

void M5StatusDisplay::drawPack(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto battery = store.battery();
  canvas_.fillSprite(kColorBg);

  char soc[16] = "--%";
  char volts[16] = "-- V";
  char amps[16] = "-- A";
  char watts[16] = "-- W";
  char ah[20] = "--/-- Ah";
  uint16_t ampsFill = kColorTileA;
  uint16_t wattsFill = kColorTileB;
  if (battery.valid) {
    std::snprintf(soc, sizeof(soc), "%u%%", battery.stateOfChargePercent);
    std::snprintf(volts, sizeof(volts), "%.1fV", static_cast<double>(battery.moduleVoltage));
    std::snprintf(amps, sizeof(amps), "%.1fA", static_cast<double>(battery.currentAmps));
    std::snprintf(watts, sizeof(watts), "%.0fW", static_cast<double>(battery.powerWatts));
    std::snprintf(ah, sizeof(ah), "%.1f/%.1f", static_cast<double>(battery.remainingCapacityAh),
                  static_cast<double>(battery.totalCapacityAh));
    ampsFill = signedTileFill(battery.currentAmps, kColorTileA);
    wattsFill = signedTileFill(battery.powerWatts, kColorTileB);
  }

  drawTile(canvas_, 8, 8, 148, 58, kColorTileC, "SOC", soc);
  drawTile(canvas_, 164, 8, 148, 58, kColorTileD, "VOLTAGE", volts);
  drawTile(canvas_, 8, 74, 148, 58, ampsFill, "CURRENT", amps);
  drawTile(canvas_, 164, 74, 148, 58, wattsFill, "POWER", watts);
  drawTile(canvas_, 8, 140, 304, 52, kColorTileA, "CAPACITY Ah", ah);

  drawButtonFooter("Pack");
  pushFrame();
#else
  (void)store;
#endif
}

void M5StatusDisplay::drawSolar(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto solar = store.solar();
  canvas_.fillSprite(kColorBg);

  char volts[16] = "-- V";
  char amps[16] = "-- A";
  char watts[16] = "-- W";
  char energy[16] = "-- Wh";
  char spp[16] = "no link";
  uint16_t ampsFill = kColorTileA;
  uint16_t wattsFill = kColorTileB;
  uint16_t sppFill = kColorBad;
  if (solar.linkFresh) {
    std::snprintf(volts, sizeof(volts), "%.1fV", static_cast<double>(solar.voltageV));
    std::snprintf(amps, sizeof(amps), "%.2fA", static_cast<double>(solar.currentA));
    std::snprintf(watts, sizeof(watts), "%.1fW", static_cast<double>(solar.powerW));
    std::snprintf(energy, sizeof(energy), "%.1fWh", static_cast<double>(solar.energyWh));
    std::snprintf(spp, sizeof(spp), "%s", solar.sppConnected ? "SPP OK" : "SPP down");
    ampsFill = signedTileFill(solar.currentA, kColorTileA);
    wattsFill = signedTileFill(solar.powerW, kColorTileB);
    sppFill = statusTileFill(solar.sppConnected);
  }

  drawTile(canvas_, 8, 8, 148, 58, kColorTileC, "SOLAR V", volts);
  drawTile(canvas_, 164, 8, 148, 58, ampsFill, "SOLAR I", amps);
  drawTile(canvas_, 8, 74, 148, 58, wattsFill, "POWER", watts);
  drawTile(canvas_, 164, 74, 148, 58, kColorTileD, "ENERGY", energy);
  drawTile(canvas_, 8, 140, 304, 52, sppFill, "XT LINK", spp);

  drawButtonFooter("Solar");
  pushFrame();
#else
  (void)store;
#endif
}

void M5StatusDisplay::drawGateway(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto gateway = store.status();
  const auto solar = store.solar();
  canvas_.fillSprite(kColorBg);

  char wifi[16] = "DOWN";
  if (gateway.wifiConnected) {
    std::snprintf(wifi, sizeof(wifi), "OK");
  }
  char ip[16] = "--";
  if (gateway.wifiIp[0] != '\0') {
    std::snprintf(ip, sizeof(ip), "%s", gateway.wifiIp);
  }
  char web[12] = {};
  std::snprintf(web, sizeof(web), ":%u", gateway.webPort);
  char bleValue[12] = {};
  std::snprintf(bleValue, sizeof(bleValue), "%s", gateway.bleConnected ? "OK" : "DOWN");
  char ntpValue[16] = {};
  std::snprintf(ntpValue, sizeof(ntpValue), "%s", gateway.ntpSynced ? "synced" : "pending");
  char xtValue[16] = {};
  std::snprintf(xtValue, sizeof(xtValue), "%s", solar.linkFresh ? "fresh" : "stale");

  drawTile(canvas_, 8, 8, 148, 58, statusTileFill(gateway.wifiConnected), "WiFi", wifi);
  drawTile(canvas_, 164, 8, 148, 58, kColorTileA, "IP", ip, 1);
  drawTile(canvas_, 8, 74, 148, 58, statusTileFill(gateway.bleConnected), "BMS BLE", bleValue);
  drawTile(canvas_, 164, 74, 148, 58, warnTileFill(gateway.ntpSynced), "NTP", ntpValue);
  drawTile(canvas_, 8, 140, 148, 52, warnTileFill(solar.linkFresh), "XT LINK", xtValue);
  drawTile(canvas_, 164, 140, 148, 52, kColorTileC, "WEB", web);

  drawButtonFooter("Gateway");
  pushFrame();
#else
  (void)store;
#endif
}

void M5StatusDisplay::drawEsp(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto esp = store.espHealth();
  canvas_.fillSprite(kColorBg);

  char cpu0[12] = {};
  char cpu1[12] = {};
  char temp[16] = {};
  char heap[16] = {};
  char up[16] = {};
  std::snprintf(cpu0, sizeof(cpu0), "%u%%", esp.cpuCore0Percent);
  std::snprintf(cpu1, sizeof(cpu1), "%u%%", esp.cpuCore1Percent);
  std::snprintf(temp, sizeof(temp), "%.1fC", static_cast<double>(esp.chipTemperatureC));
  std::snprintf(heap, sizeof(heap), "%uKB", esp.freeHeapBytes / 1024u);
  std::snprintf(up, sizeof(up), "%us", esp.uptimeSeconds);

  drawTile(canvas_, 8, 8, 148, 58, kColorTileA, "CPU 0", cpu0);
  drawTile(canvas_, 164, 8, 148, 58, kColorTileB, "CPU 1", cpu1);
  drawTile(canvas_, 8, 74, 148, 58, kColorTileC, "CHIP", temp);
  drawTile(canvas_, 164, 74, 148, 58, kColorTileD, "HEAP", heap);

  char bridge[20] = "offline";
  uint16_t bridgeFill = kColorBad;
  if (store.bmsBridgeHealthValid()) {
    const auto b = store.bmsBridgeHealth();
    std::snprintf(bridge, sizeof(bridge), "%u%% %uKB", b.cpuCore0Percent,
                  b.freeHeapBytes / 1024u);
    bridgeFill = kColorOk;
  }
  drawTile(canvas_, 8, 140, 148, 52, kColorTileB, "UPTIME", up);
  drawTile(canvas_, 164, 140, 148, 52, bridgeFill, "BMS BRIDGE", bridge, 1);

  drawButtonFooter("ESP");
  pushFrame();
#else
  (void)store;
#endif
}

void M5StatusDisplay::drawTemps(const telemetry::ITelemetryStore& store) {
#ifndef UNIT_TEST
  const auto battery = store.battery();
  canvas_.fillSprite(kColorBg);

  char mos[16] = "-- C";
  char pcb[16] = "-- C";
  char t1[16] = "--";
  char t2[16] = "--";
  char t3[16] = "--";
  char t4[16] = "--";
  if (battery.valid) {
    std::snprintf(mos, sizeof(mos), "%.1fC", static_cast<double>(battery.mosTemperatureC));
    std::snprintf(pcb, sizeof(pcb), "%.1fC", static_cast<double>(battery.pcbTemperatureC));
    const uint8_t sensors =
        battery.temperatureCount > 2 ? static_cast<uint8_t>(battery.temperatureCount - 2) : 0;
    if (sensors > 0) {
      std::snprintf(t1, sizeof(t1), "%.1fC", static_cast<double>(battery.cellTemperaturesC[0]));
    }
    if (sensors > 1) {
      std::snprintf(t2, sizeof(t2), "%.1fC", static_cast<double>(battery.cellTemperaturesC[1]));
    }
    if (sensors > 2) {
      std::snprintf(t3, sizeof(t3), "%.1fC", static_cast<double>(battery.cellTemperaturesC[2]));
    }
    if (sensors > 3) {
      std::snprintf(t4, sizeof(t4), "%.1fC", static_cast<double>(battery.cellTemperaturesC[3]));
    }
  }

  drawTile(canvas_, 8, 8, 148, 58, kColorTileC, "MOS", mos);
  drawTile(canvas_, 164, 8, 148, 58, kColorTileB, "PCB", pcb);
  drawTile(canvas_, 8, 74, 148, 58, kColorTileA, "CELL T1", t1);
  drawTile(canvas_, 164, 74, 148, 58, kColorTileD, "CELL T2", t2);
  drawTile(canvas_, 8, 140, 148, 52, kColorTileB, "CELL T3", t3);
  drawTile(canvas_, 164, 140, 148, 52, kColorTileC, "CELL T4", t4);

  drawButtonFooter("Temps");
  pushFrame();
#else
  (void)store;
#endif
}

void M5StatusDisplay::render(const telemetry::ITelemetryStore& store) {
  if (!ready_) {
    return;
  }
  switch (pageIndex_) {
    case 0:
      drawOverview(store);
      break;
    case 1:
      drawPack(store);
      break;
    case 2:
      drawSolar(store);
      break;
    case 3:
      drawGateway(store);
      break;
    case 4:
      drawEsp(store);
      break;
    case 5:
      drawTemps(store);
      break;
    default:
      pageIndex_ = 0;
      drawOverview(store);
      break;
  }
}

void M5StatusDisplay::renderAuthPrompt(const auth::AuthPrompt& prompt) {
#ifndef UNIT_TEST
  if (!ready_ || prompt.kind == auth::AuthPromptKind::None) {
    return;
  }
  canvas_.fillSprite(kColorBg);
  if (prompt.kind == auth::AuthPromptKind::ConfirmSetup) {
    drawTile(canvas_, 8, 8, 304, 72, kColorTileC, "CONFIRM LOGIN", prompt.username);
    drawTile(canvas_, 8, 92, 304, 58, kColorOk, "ACTION", "Press B to OK");
  } else {
    drawTile(canvas_, 8, 8, 304, 72, kColorTileC, "PASSWORD RESET", "Confirm on device");
    drawTile(canvas_, 8, 92, 304, 58, kColorOk, "ACTION", "Press B to OK");
  }
  drawButtonFooter("OK");
  pushFrame();
#else
  (void)prompt;
#endif
}

}  // namespace wattcycle::display
