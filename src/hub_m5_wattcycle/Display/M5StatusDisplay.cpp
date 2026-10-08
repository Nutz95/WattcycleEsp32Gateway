#include "Display/M5StatusDisplay.h"

#include "Bus/SharedSpiLock.h"
#include "Display/DisplayPins.h"

#include <cstdio>

namespace wattcycle::display {
namespace {
#ifndef UNIT_TEST
constexpr uint16_t kColorBg = TFT_BLACK;
constexpr uint16_t kColorTileA = 0x1A4B;  // deep blue
constexpr uint16_t kColorTileB = 0x2284;  // teal
constexpr uint16_t kColorTileC = 0x5120;  // amber-ish
constexpr uint16_t kColorTileD = 0x4147;  // purple-gray
constexpr uint16_t kColorFooterTile = 0x2124;
constexpr uint16_t kColorMuted = 0x8410;

void drawTile(M5Canvas& canvas, int x, int y, int w, int h, uint16_t fill,
              const char* title, const char* value) {
  canvas.fillRoundRect(x, y, w, h, 6, fill);
  canvas.setTextColor(kColorMuted, fill);
  canvas.setTextSize(1);
  canvas.setCursor(x + 8, y + 8);
  canvas.print(title != nullptr ? title : "");
  canvas.setTextColor(TFT_WHITE, fill);
  canvas.setTextSize(2);
  canvas.setCursor(x + 8, y + 28);
  canvas.print(value != nullptr ? value : "--");
}

void drawWideTile(M5Canvas& canvas, int x, int y, int w, int h, uint16_t fill,
                  const char* title, const char* value) {
  drawTile(canvas, x, y, w, h, fill, title, value);
}

void drawFooterTile(M5Canvas& canvas, int x, int y, int w, int h, const char* top,
                    const char* bottom) {
  canvas.fillRoundRect(x, y, w, h, 6, kColorFooterTile);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, kColorFooterTile);
  canvas.setCursor(x + 10, y + 6);
  canvas.print(top != nullptr ? top : "");
  canvas.setTextColor(kColorMuted, kColorFooterTile);
  canvas.setCursor(x + 10, y + 20);
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
  char pageLabel[12] = {};
  std::snprintf(pageLabel, sizeof(pageLabel), "%u/%u", static_cast<unsigned>(pageIndex_ + 1),
                static_cast<unsigned>(kPageCount));
  drawFooterTile(canvas_, 8, y, tileW, h, "A  <", "prev");
  drawFooterTile(canvas_, 8 + tileW + gap, y, tileW, h,
                 centerLabel != nullptr ? centerLabel : "B", pageLabel);
  drawFooterTile(canvas_, 8 + (tileW + gap) * 2, y, tileW, h, ">  C", "next");
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
  if (solar.linkFresh) {
    std::snprintf(solarPower, sizeof(solarPower), "%.0fW", static_cast<double>(solar.powerW));
  }
  char wifi[16] = "WiFi DOWN";
  if (gateway.wifiConnected) {
    std::snprintf(wifi, sizeof(wifi), "WiFi OK");
  }
  char clockTitle[16] = "CLOCK";
  char clockValue[16] = "NTP wait";
  if (gateway.ntpSynced && gateway.localTime[0] != '\0') {
    std::snprintf(clockTitle, sizeof(clockTitle), "%s", gateway.localDate);
    std::snprintf(clockValue, sizeof(clockValue), "%s", gateway.localTime);
  }

  drawTile(canvas_, 8, 8, 148, 72, kColorTileA, "PACK SOC", soc);
  drawTile(canvas_, 164, 8, 148, 72, kColorTileB, "SOLAR", solarPower);
  drawTile(canvas_, 8, 88, 148, 72, kColorTileC, "NETWORK", wifi);
  drawTile(canvas_, 164, 88, 148, 72, kColorTileD, clockTitle, clockValue);

  drawButtonFooter("B");
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
  if (battery.valid) {
    std::snprintf(soc, sizeof(soc), "%u%%", battery.stateOfChargePercent);
    std::snprintf(volts, sizeof(volts), "%.1fV", static_cast<double>(battery.moduleVoltage));
    std::snprintf(amps, sizeof(amps), "%.1fA", static_cast<double>(battery.currentAmps));
    std::snprintf(watts, sizeof(watts), "%.0fW", static_cast<double>(battery.powerWatts));
    std::snprintf(ah, sizeof(ah), "%.1f/%.1f", static_cast<double>(battery.remainingCapacityAh),
                  static_cast<double>(battery.totalCapacityAh));
  }

  drawTile(canvas_, 8, 8, 148, 58, kColorTileA, "SOC", soc);
  drawTile(canvas_, 164, 8, 148, 58, kColorTileB, "VOLTAGE", volts);
  drawTile(canvas_, 8, 74, 148, 58, kColorTileC, "CURRENT", amps);
  drawTile(canvas_, 164, 74, 148, 58, kColorTileD, "POWER", watts);
  drawWideTile(canvas_, 8, 140, 304, 52, kColorTileA, "CAPACITY Ah", ah);

  drawButtonFooter("B");
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
  if (solar.linkFresh) {
    std::snprintf(volts, sizeof(volts), "%.1fV", static_cast<double>(solar.voltageV));
    std::snprintf(amps, sizeof(amps), "%.2fA", static_cast<double>(solar.currentA));
    std::snprintf(watts, sizeof(watts), "%.1fW", static_cast<double>(solar.powerW));
    std::snprintf(energy, sizeof(energy), "%.1fWh", static_cast<double>(solar.energyWh));
    std::snprintf(spp, sizeof(spp), "%s", solar.sppConnected ? "SPP OK" : "SPP down");
  }

  drawTile(canvas_, 8, 8, 148, 58, kColorTileB, "SOLAR V", volts);
  drawTile(canvas_, 164, 8, 148, 58, kColorTileA, "SOLAR I", amps);
  drawTile(canvas_, 8, 74, 148, 58, kColorTileC, "POWER", watts);
  drawTile(canvas_, 164, 74, 148, 58, kColorTileD, "ENERGY", energy);
  drawWideTile(canvas_, 8, 140, 304, 52, solar.sppConnected ? kColorTileB : kColorTileC, "XT LINK",
               spp);

  drawButtonFooter("B");
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

  drawTile(canvas_, 8, 8, 148, 58, kColorTileC, "WiFi", wifi);
  drawTile(canvas_, 164, 8, 148, 58, kColorTileA, "IP", ip);
  drawTile(canvas_, 8, 74, 148, 58, kColorTileB, "BMS BLE", bleValue);
  drawTile(canvas_, 164, 74, 148, 58, kColorTileD, "NTP", ntpValue);
  drawTile(canvas_, 8, 140, 148, 52, kColorTileA, "XT LINK", xtValue);
  drawTile(canvas_, 164, 140, 148, 52, kColorTileC, "WEB", web);

  drawButtonFooter("B");
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

  char bridge[20] = "no BMS ESP";
  if (store.bmsBridgeHealthValid()) {
    const auto b = store.bmsBridgeHealth();
    std::snprintf(bridge, sizeof(bridge), "%u%% %uKB", b.cpuCore0Percent,
                  b.freeHeapBytes / 1024u);
  }
  drawTile(canvas_, 8, 140, 148, 52, kColorTileB, "UPTIME", up);
  drawTile(canvas_, 164, 140, 148, 52, kColorTileA, "BMS BRIDGE", bridge);

  drawButtonFooter("B");
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

  drawButtonFooter("B");
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
    drawWideTile(canvas_, 8, 8, 304, 72, kColorTileC, "CONFIRM LOGIN", prompt.username);
    drawWideTile(canvas_, 8, 92, 304, 58, kColorTileA, "ACTION", "Press B to OK");
  } else {
    drawWideTile(canvas_, 8, 8, 304, 72, kColorTileC, "PASSWORD RESET", "Confirm on device");
    drawWideTile(canvas_, 8, 92, 304, 58, kColorTileA, "ACTION", "Press B to OK");
  }
  drawButtonFooter("OK");
  pushFrame();
#else
  (void)prompt;
#endif
}

}  // namespace wattcycle::display
