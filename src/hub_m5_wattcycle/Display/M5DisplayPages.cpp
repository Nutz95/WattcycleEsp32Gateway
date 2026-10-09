#include "Display/M5DisplayPages.h"

#include "Display/DisplayPins.h"
#include "Display/M5DisplayTiles.h"
#include "Telemetry/ITelemetryStore.h"

#include <cstdio>
#include <cstring>

#ifndef UNIT_TEST

namespace wattcycle::display {

namespace {
constexpr int kRowGap = 4;
constexpr int kRowH = 45;
constexpr int kRowY0 = 8;
constexpr int kRowY1 = kRowY0 + kRowH + kRowGap;
constexpr int kRowY2 = kRowY1 + kRowH + kRowGap;
constexpr int kRowY3 = kRowY2 + kRowH + kRowGap;
constexpr int kColW = 148;
constexpr int kColLeft = 8;
constexpr int kColRight = 164;
}  // namespace

const char* const kPageNames[kPageCount] = {"Overview", "EcoFlow", "Pack", "Solar",
                                            "Gateway", "ESP", "Temps"};

void drawM5ButtonFooter(M5Canvas& canvas, uint8_t pageIndex, const char* centerLabel) {
  const int y = kDisplayHeight - kFooterHeight + 2;
  const int h = kFooterHeight - 4;
  const int gap = 4;
  const int tileW = (kDisplayWidth - 16 - gap * 2) / 3;
  const bool authMode = centerLabel != nullptr && std::strcmp(centerLabel, "OK") == 0;
  const char* midTop =
      authMode ? "OK" : ((pageIndex < kPageCount) ? kPageNames[pageIndex] : "Page");
  const char* midBottom = authMode ? "confirm" : "B";
  char nextLine[16] = {};
  std::snprintf(nextLine, sizeof(nextLine), "next %u/%u", static_cast<unsigned>(pageIndex + 1),
                static_cast<unsigned>(kPageCount));
  drawFooterTile(canvas, 8, y, tileW, h, "A  <", "prev");
  drawFooterTile(canvas, 8 + tileW + gap, y, tileW, h, midTop, midBottom);
  drawFooterTile(canvas, 8 + (tileW + gap) * 2, y, tileW, h, ">  C", nextLine);
}

void drawM5OverviewPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store) {
  const auto battery = store.battery();
  const auto solar = store.solar();
  const auto gateway = store.status();
  const auto ecoflow = store.ecoflow();
  canvas.fillSprite(kColorBg);

  char efSoc[16] = "--%";
  uint16_t efSocFill = kColorBad;
  if (ecoflow.linkFresh && ecoflow.meterValid) {
    std::snprintf(efSoc, sizeof(efSoc), "%u%%", ecoflow.socPercent);
    efSocFill = kColorTileD;
  } else if (ecoflow.linkFresh) {
    std::snprintf(efSoc, sizeof(efSoc), "wait");
    efSocFill = kColorWarn;
  }

  char packSoc[20] = "--%";
  if (battery.valid) {
    std::snprintf(packSoc, sizeof(packSoc), "%u%% %.0fAh", battery.stateOfChargePercent,
                  static_cast<double>(battery.remainingCapacityAh));
  }

  char efPower[16] = "-- W";
  uint16_t efPowerFill = kColorTileC;
  if (ecoflow.linkFresh && ecoflow.meterValid) {
    std::snprintf(efPower, sizeof(efPower), "%dW", static_cast<int>(ecoflow.acOutputW));
    efPowerFill = signedTileFill(static_cast<float>(ecoflow.acOutputW), kColorTileC);
  } else if (!ecoflow.linkFresh) {
    std::snprintf(efPower, sizeof(efPower), "stale");
    efPowerFill = kColorBad;
  }

  char packPower[16] = "-- W";
  uint16_t packPowerFill = kColorTileB;
  if (battery.valid) {
    std::snprintf(packPower, sizeof(packPower), "%.0fW", static_cast<double>(battery.powerWatts));
    packPowerFill = signedTileFill(battery.powerWatts, kColorTileB);
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

  drawTile(canvas, kColLeft, kRowY0, kColW, kRowH, efSocFill, "EF SOC", efSoc, 1);
  drawTile(canvas, kColRight, kRowY0, kColW, kRowH, kColorTileA, "PACK SOC", packSoc, 1);
  drawTile(canvas, kColLeft, kRowY1, kColW, kRowH, efPowerFill, "EF AC OUT", efPower, 1);
  drawTile(canvas, kColRight, kRowY1, kColW, kRowH, packPowerFill, "PACK W", packPower, 1);
  drawTile(canvas, kColLeft, kRowY2, kColW, kRowH, solarFill, "SOLAR", solarPower, 1);
  drawTile(canvas, kColRight, kRowY2, kColW, kRowH, statusTileFill(gateway.wifiConnected),
           "NETWORK", wifi, 1);
  drawTile(canvas, kColLeft, kRowY3, 304, kRowH, clockFill, clockTitle, clockValue, 1);
}

void drawM5EcoflowPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store) {
  const auto ecoflow = store.ecoflow();
  canvas.fillSprite(kColorBg);

  char soc[16] = "--%";
  char acIn[16] = "-- W";
  char acOut[16] = "-- W";
  char dcOut[16] = "-- W";
  char usbOut[16] = "-- W";
  char remain[16] = "-- min";
  char ports[16] = "A D U";
  char portState[16] = "-- -- --";
  char ble[16] = "stale";
  uint16_t socFill = kColorBad;
  uint16_t acOutFill = kColorTileC;
  uint16_t dcOutFill = kColorTileA;
  uint16_t bleFill = kColorBad;

  if (ecoflow.linkFresh) {
    bleFill = statusTileFill(ecoflow.bleConnected);
    std::snprintf(ble, sizeof(ble), "%s", ecoflow.bleConnected ? "OK" : "DOWN");
    if (ecoflow.meterValid) {
      socFill = kColorTileD;
      std::snprintf(soc, sizeof(soc), "%u%%", ecoflow.socPercent);
      std::snprintf(acIn, sizeof(acIn), "%dW", static_cast<int>(ecoflow.acInputW));
      std::snprintf(acOut, sizeof(acOut), "%dW", static_cast<int>(ecoflow.acOutputW));
      std::snprintf(dcOut, sizeof(dcOut), "%dW", static_cast<int>(ecoflow.dcOutputW));
      std::snprintf(usbOut, sizeof(usbOut), "%dW", static_cast<int>(ecoflow.usbOutputW));
      std::snprintf(remain, sizeof(remain), "%u min", ecoflow.remainMinutes);
      acOutFill = signedTileFill(static_cast<float>(ecoflow.acOutputW), kColorTileC);
      dcOutFill = signedTileFill(static_cast<float>(ecoflow.dcOutputW), kColorTileA);
      std::snprintf(portState, sizeof(portState), "%s %s %s",
                    ecoflow.acOutputOn ? "ON" : "off", ecoflow.dcOutputOn ? "ON" : "off",
                    ecoflow.usbOutputOn ? "ON" : "off");
    } else {
      std::snprintf(soc, sizeof(soc), "wait");
      socFill = kColorWarn;
    }
  }

  drawTile(canvas, kColLeft, kRowY0, kColW, kRowH, socFill, "SOC", soc, 1);
  drawTile(canvas, kColRight, kRowY0, kColW, kRowH, kColorTileB, "AC IN", acIn, 1);
  drawTile(canvas, kColLeft, kRowY1, kColW, kRowH, acOutFill, "AC OUT", acOut, 1);
  drawTile(canvas, kColRight, kRowY1, kColW, kRowH, dcOutFill, "DC OUT", dcOut, 1);
  drawTile(canvas, kColLeft, kRowY2, kColW, kRowH, kColorTileD, "USB OUT", usbOut, 1);
  drawTile(canvas, kColRight, kRowY2, kColW, kRowH, kColorTileA, "REMAIN", remain, 1);
  drawTile(canvas, kColLeft, kRowY3, kColW, kRowH, kColorTileB, ports, portState, 1);
  drawTile(canvas, kColRight, kRowY3, kColW, kRowH, bleFill, "BLE LINK", ble, 1);
}

void drawM5PackPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store) {
  const auto battery = store.battery();
  canvas.fillSprite(kColorBg);

  char soc[16] = "--%";
  char volts[16] = "-- V";
  char amps[16] = "-- A";
  char watts[16] = "-- W";
  char ah[24] = "-- / -- Ah";
  uint16_t ampsFill = kColorTileA;
  uint16_t wattsFill = kColorTileB;
  if (battery.valid) {
    std::snprintf(soc, sizeof(soc), "%u%%", battery.stateOfChargePercent);
    std::snprintf(volts, sizeof(volts), "%.1fV", static_cast<double>(battery.moduleVoltage));
    std::snprintf(amps, sizeof(amps), "%.1fA", static_cast<double>(battery.currentAmps));
    std::snprintf(watts, sizeof(watts), "%.0fW", static_cast<double>(battery.powerWatts));
    std::snprintf(ah, sizeof(ah), "%.1f / %.1f Ah",
                  static_cast<double>(battery.remainingCapacityAh),
                  static_cast<double>(battery.totalCapacityAh));
    ampsFill = signedTileFill(battery.currentAmps, kColorTileA);
    wattsFill = signedTileFill(battery.powerWatts, kColorTileB);
  }

  drawTile(canvas, 8, 8, 148, 58, kColorTileC, "SOC", soc);
  drawTile(canvas, 164, 8, 148, 58, kColorTileD, "VOLTAGE", volts);
  drawTile(canvas, 8, 74, 148, 58, ampsFill, "CURRENT", amps);
  drawTile(canvas, 164, 74, 148, 58, wattsFill, "POWER", watts);
  drawTile(canvas, 8, 140, 304, 52, kColorTileA, "CAPACITY", ah, 1);
}

void drawM5SolarPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store) {
  const auto solar = store.solar();
  canvas.fillSprite(kColorBg);

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

  drawTile(canvas, 8, 8, 148, 58, kColorTileC, "SOLAR V", volts);
  drawTile(canvas, 164, 8, 148, 58, ampsFill, "SOLAR I", amps);
  drawTile(canvas, 8, 74, 148, 58, wattsFill, "POWER", watts);
  drawTile(canvas, 164, 74, 148, 58, kColorTileD, "ENERGY", energy);
  drawTile(canvas, 8, 140, 304, 52, sppFill, "XT LINK", spp);
}

void drawM5GatewayPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store) {
  const auto gateway = store.status();
  const auto solar = store.solar();
  const auto ecoflow = store.ecoflow();
  canvas.fillSprite(kColorBg);

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
  char efValue[16] = {};
  std::snprintf(efValue, sizeof(efValue), "%s", ecoflow.linkFresh ? "fresh" : "stale");

  drawTile(canvas, kColLeft, kRowY0, kColW, kRowH, statusTileFill(gateway.wifiConnected), "WiFi",
           wifi, 1);
  drawTile(canvas, kColRight, kRowY0, kColW, kRowH, kColorTileA, "IP", ip, 1);
  drawTile(canvas, kColLeft, kRowY1, kColW, kRowH, statusTileFill(gateway.bleConnected),
           "BMS BLE", bleValue, 1);
  drawTile(canvas, kColRight, kRowY1, kColW, kRowH, warnTileFill(gateway.ntpSynced), "NTP",
           ntpValue, 1);
  drawTile(canvas, kColLeft, kRowY2, kColW, kRowH, warnTileFill(solar.linkFresh), "XT LINK",
           xtValue, 1);
  drawTile(canvas, kColRight, kRowY2, kColW, kRowH, warnTileFill(ecoflow.linkFresh), "EF LINK",
           efValue, 1);
  drawTile(canvas, kColLeft, kRowY3, 304, kRowH, kColorTileC, "WEB", web, 1);
}

void drawM5EspPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store) {
  const auto esp = store.espHealth();
  canvas.fillSprite(kColorBg);

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

  drawTile(canvas, 8, 8, 148, 58, kColorTileA, "CPU 0", cpu0);
  drawTile(canvas, 164, 8, 148, 58, kColorTileB, "CPU 1", cpu1);
  drawTile(canvas, 8, 74, 148, 58, kColorTileC, "CHIP", temp);
  drawTile(canvas, 164, 74, 148, 58, kColorTileD, "HEAP", heap);

  char bridge[20] = "offline";
  uint16_t bridgeFill = kColorBad;
  if (store.bmsBridgeHealthValid()) {
    const auto b = store.bmsBridgeHealth();
    std::snprintf(bridge, sizeof(bridge), "%u%% %uKB", b.cpuCore0Percent,
                  b.freeHeapBytes / 1024u);
    bridgeFill = kColorOk;
  }
  drawTile(canvas, 8, 140, 148, 52, kColorTileB, "UPTIME", up);
  drawTile(canvas, 164, 140, 148, 52, bridgeFill, "BMS BRIDGE", bridge, 1);
}

void drawM5TempsPage(M5Canvas& canvas, const telemetry::ITelemetryStore& store) {
  const auto battery = store.battery();
  canvas.fillSprite(kColorBg);

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

  drawTile(canvas, 8, 8, 148, 58, kColorTileC, "MOS", mos);
  drawTile(canvas, 164, 8, 148, 58, kColorTileB, "PCB", pcb);
  drawTile(canvas, 8, 74, 148, 58, kColorTileA, "CELL T1", t1);
  drawTile(canvas, 164, 74, 148, 58, kColorTileD, "CELL T2", t2);
  drawTile(canvas, 8, 140, 148, 52, kColorTileB, "CELL T3", t3);
  drawTile(canvas, 164, 140, 148, 52, kColorTileC, "CELL T4", t4);
}

void drawM5AuthPromptPage(M5Canvas& canvas, const auth::AuthPrompt& prompt) {
  canvas.fillSprite(kColorBg);
  if (prompt.kind == auth::AuthPromptKind::ConfirmSetup) {
    drawTile(canvas, 8, 8, 304, 72, kColorTileC, "CONFIRM LOGIN", prompt.username);
    drawTile(canvas, 8, 92, 304, 58, kColorOk, "ACTION", "Press B to OK");
  } else {
    drawTile(canvas, 8, 8, 304, 72, kColorTileC, "PASSWORD RESET", "Confirm on device");
    drawTile(canvas, 8, 92, 304, 58, kColorOk, "ACTION", "Press B to OK");
  }
}

}  // namespace wattcycle::display

#endif
