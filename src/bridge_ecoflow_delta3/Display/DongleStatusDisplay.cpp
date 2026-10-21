#include "Display/DongleStatusDisplay.h"

#include "EcoFlow/Protocol/EcoFlowDisplayPropertyDecoder.h"
#include "boards/dongle_s3/DongleEspHealthLines.h"
#include "boards/dongle_s3/DonglePanelPalette.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstdlib>
#include <cstdio>

namespace ecoflow::display {
namespace {

using boards::dongle_s3::DonglePanelPalette;
using ecoflow::protocol::EcoFlowDisplayPropertyDecoder;

const char* kPageTitles[7] = {"EF Overview", "EF Inputs", "EF Outputs", "EF Pack",
                               "EF Meter",    "EF Link",   "EF ESP"};

void textLine(TFT_eSprite& canvas, int y, const char* text, uint16_t color) {
  canvas.setTextColor(color, DonglePanelPalette::kBackground);
  canvas.setTextSize(1);
  canvas.drawString(text, 4, y);
}

void formatRemain(char* buffer, size_t bufferSize, uint16_t minutes) {
  if (minutes == 0) {
    std::snprintf(buffer, bufferSize, "time --");
    return;
  }
  if (minutes >= 60) {
    std::snprintf(buffer, bufferSize, "%uh%02u", minutes / 60u, minutes % 60u);
    return;
  }
  std::snprintf(buffer, bufferSize, "%umin", minutes);
}

}  // namespace

void DongleStatusDisplay::begin() {
  panel_.begin("ecoflow-ui", "ECOFLOW");
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

void DongleStatusDisplay::drawOverview(const ble::EcoFlowBleSnapshot& bleSnapshot,
                                       const DongleBridgeLink& link) {
  panel_.drawChrome(kPageTitles[0], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto& telemetry = bleSnapshot.telemetry;
  char line[40];
  if (!telemetry.valid) {
    textLine(canvas, 20, bleSnapshot.connected ? "Waiting data..." : "BLE down",
             DonglePanelPalette::kWarn);
    textLine(canvas, 34, bleSnapshot.lastError[0] ? bleSnapshot.lastError : "no frame yet",
             DonglePanelPalette::kMuted);
  } else {
    char remain[16];
    formatRemain(remain, sizeof(remain), telemetry.remainMinutes);
    std::snprintf(line, sizeof(line), "%u%%  %s", telemetry.socPercent, remain);
    textLine(canvas, 18, line, DonglePanelPalette::kAccent);
    std::snprintf(line, sizeof(line), "in %dW  PV %dW", telemetry.acInputW, telemetry.solarInputW);
    textLine(canvas, 32, line, DonglePanelPalette::kText);
    const int outW = telemetry.acOutputW + telemetry.dcOutputW + telemetry.usbOutputW;
    if (telemetry.haveTemperature) {
      std::snprintf(line, sizeof(line), "out %dW  %dC", outW, telemetry.tempC);
    } else {
      std::snprintf(line, sizeof(line), "out %dW", outW);
    }
    textLine(canvas, 46, line, DonglePanelPalette::kText);
  }
  std::snprintf(line, sizeof(line), "%s  %s  ch%u", bleSnapshot.connected ? "BLE" : "BLE!",
                link.espNowReady ? "NOW" : "NOW!", link.channel);
  textLine(canvas, 62, line,
           (bleSnapshot.connected && link.espNowReady) ? DonglePanelPalette::kAccent
                                                       : DonglePanelPalette::kDanger);
}

void DongleStatusDisplay::drawInputs(const ble::EcoFlowBleSnapshot& bleSnapshot) {
  panel_.drawChrome(kPageTitles[1], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto& telemetry = bleSnapshot.telemetry;
  char line[32];
  if (!telemetry.valid) {
    textLine(canvas, 28, "No input data", DonglePanelPalette::kMuted);
    return;
  }
  if (telemetry.haveFreq) {
    std::snprintf(line, sizeof(line), "AC in %dW %uHz", telemetry.acInputW, telemetry.acInFreqHz);
  } else {
    std::snprintf(line, sizeof(line), "AC in %dW", telemetry.acInputW);
  }
  textLine(canvas, 20, line, telemetry.acInputOn ? DonglePanelPalette::kAccent
                                                 : DonglePanelPalette::kText);
  std::snprintf(line, sizeof(line), "PV %s %dW", telemetry.pvInputOn ? "on" : "off",
                telemetry.solarInputW);
  textLine(canvas, 36, line, telemetry.solarInputW > 0 ? DonglePanelPalette::kAccent
                                                       : DonglePanelPalette::kText);
  std::snprintf(line, sizeof(line), "sum %dW", telemetry.inSumW);
  textLine(canvas, 52, line, DonglePanelPalette::kMuted);
}

void DongleStatusDisplay::drawOutputs(const ble::EcoFlowBleSnapshot& bleSnapshot) {
  panel_.drawChrome(kPageTitles[2], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto& telemetry = bleSnapshot.telemetry;
  char line[36];
  std::snprintf(line, sizeof(line), "AC  %s  %dW", telemetry.acOutputOn ? "ON" : "off",
                telemetry.acOutputW);
  textLine(canvas, 20, line,
           telemetry.acOutputOn ? DonglePanelPalette::kAccent : DonglePanelPalette::kMuted);
  std::snprintf(line, sizeof(line), "DC  %s  %dW", telemetry.dcOutputOn ? "ON" : "off",
                telemetry.dcOutputW);
  textLine(canvas, 36, line,
           telemetry.dcOutputOn ? DonglePanelPalette::kAccent : DonglePanelPalette::kMuted);
  std::snprintf(line, sizeof(line), "USB %s %dW", telemetry.usbOutputOn ? "ON" : "off",
                telemetry.usbOutputW);
  textLine(canvas, 48, line,
           telemetry.usbOutputOn ? DonglePanelPalette::kAccent : DonglePanelPalette::kMuted);
  std::snprintf(line, sizeof(line), "sum %dW", telemetry.outSumW);
  textLine(canvas, 62, line, DonglePanelPalette::kText);
}

void DongleStatusDisplay::drawPack(const ble::EcoFlowBleSnapshot& bleSnapshot) {
  panel_.drawChrome(kPageTitles[3], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto& telemetry = bleSnapshot.telemetry;
  char line[40];
  if (!telemetry.valid) {
    textLine(canvas, 28, "No pack data", DonglePanelPalette::kMuted);
    return;
  }
  if (telemetry.haveSoh && telemetry.haveFan) {
    std::snprintf(line, sizeof(line), "SOH %u%%  fan %u", telemetry.sohPercent, telemetry.fanLevel);
  } else if (telemetry.haveSoh) {
    std::snprintf(line, sizeof(line), "SOH %u%%", telemetry.sohPercent);
  } else {
    std::snprintf(line, sizeof(line), "SOH --");
  }
  textLine(canvas, 18, line, DonglePanelPalette::kAccent);
  if (telemetry.haveCellTemp) {
    std::snprintf(line, sizeof(line), "cell %d/%dC", telemetry.minCellTempC, telemetry.maxCellTempC);
  } else {
    std::snprintf(line, sizeof(line), "cell --");
  }
  textLine(canvas, 32, line, DonglePanelPalette::kText);
  if (telemetry.haveCellTemp) {
    std::snprintf(line, sizeof(line), "mos %d/%dC", telemetry.minMosTempC, telemetry.maxMosTempC);
  } else {
    std::snprintf(line, sizeof(line), "mos --");
  }
  textLine(canvas, 46, line, DonglePanelPalette::kText);
  const char* state = "--";
  if (telemetry.haveChgState) {
    state = telemetry.chgDsgState == 2 ? "chg" : (telemetry.chgDsgState == 1 ? "dsg" : "idle");
  }
  if (telemetry.haveErr) {
    std::snprintf(line, sizeof(line), "%s  err %lu", state,
                  static_cast<unsigned long>(telemetry.errCode));
  } else {
    std::snprintf(line, sizeof(line), "%s", state);
  }
  textLine(canvas, 60, line, telemetry.haveErr && telemetry.errCode != 0 ? DonglePanelPalette::kDanger
                                                                        : DonglePanelPalette::kMuted);
}

void DongleStatusDisplay::drawMeter(const ble::EcoFlowBleSnapshot& bleSnapshot) {
  panel_.drawChrome(kPageTitles[4], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  const auto& telemetry = bleSnapshot.telemetry;
  char line[40];
  if (!telemetry.haveAcInElec && !telemetry.haveBmsElec && !telemetry.haveCellVolts) {
    textLine(canvas, 28, "No meter yet", DonglePanelPalette::kMuted);
    return;
  }
  if (telemetry.haveAcInElec) {
    std::snprintf(line, sizeof(line), "in %d.%dV %d.%02dA",
                  telemetry.acInVoltsDeci / EcoFlowDisplayPropertyDecoder::kDeci,
                  std::abs(telemetry.acInVoltsDeci % EcoFlowDisplayPropertyDecoder::kDeci),
                  telemetry.acInCentiamps / EcoFlowDisplayPropertyDecoder::kCenti,
                  std::abs(telemetry.acInCentiamps % EcoFlowDisplayPropertyDecoder::kCenti));
  } else {
    std::snprintf(line, sizeof(line), "in --");
  }
  textLine(canvas, 18, line, DonglePanelPalette::kText);
  if (telemetry.haveBmsElec) {
    const int amps = telemetry.bmsCentiamps;
    const int ampMag = amps < 0 ? -amps : amps;
    std::snprintf(line, sizeof(line), "bat %d.%02dV %c%d.%02dA",
                  telemetry.bmsVoltsCenti / EcoFlowDisplayPropertyDecoder::kCenti,
                  std::abs(telemetry.bmsVoltsCenti % EcoFlowDisplayPropertyDecoder::kCenti),
                  amps < 0 ? '-' : '+', ampMag / EcoFlowDisplayPropertyDecoder::kCenti,
                  ampMag % EcoFlowDisplayPropertyDecoder::kCenti);
  } else {
    std::snprintf(line, sizeof(line), "bat --");
  }
  textLine(canvas, 34, line,
           telemetry.bmsCentiamps < 0 ? DonglePanelPalette::kDanger : DonglePanelPalette::kAccent);
  if (telemetry.haveCellVolts) {
    std::snprintf(line, sizeof(line), "cell %u-%umV", telemetry.minCellMv, telemetry.maxCellMv);
  } else if (telemetry.havePcsTemp) {
    std::snprintf(line, sizeof(line), "pcs %d/%dC", telemetry.pcsDcTempC, telemetry.pcsAcTempC);
  } else {
    std::snprintf(line, sizeof(line), "cell --");
  }
  textLine(canvas, 50, line, DonglePanelPalette::kText);
}

void DongleStatusDisplay::drawLink(const ble::EcoFlowBleSnapshot& bleSnapshot,
                                   const DongleBridgeLink& link) {
  panel_.drawChrome(kPageTitles[5], pageIndex_, kPageCount);
  TFT_eSprite& canvas = panel_.canvas();
  char line[48];
  std::snprintf(line, sizeof(line), "%s  %s", bleSnapshot.connected ? "BLE up" : "BLE down",
                bleSnapshot.authenticated ? "auth OK" : "auth --");
  textLine(canvas, 18, line,
           (bleSnapshot.connected && bleSnapshot.authenticated) ? DonglePanelPalette::kAccent
                                                               : DonglePanelPalette::kDanger);
  std::snprintf(line, sizeof(line), "NOW %s ch%u", link.espNowReady ? "OK" : "DOWN", link.channel);
  textLine(canvas, 32, line,
           link.espNowReady ? DonglePanelPalette::kAccent : DonglePanelPalette::kDanger);
  textLine(canvas, 46, link.peerMac != nullptr && link.peerMac[0] ? link.peerMac : "(no peer)",
           DonglePanelPalette::kText);
  textLine(canvas, 60, bleSnapshot.lastError[0] ? bleSnapshot.lastError : "link idle",
           DonglePanelPalette::kMuted);
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
  panel_.drawEspPage(kPageTitles[6], pageIndex_, kPageCount, lines);
}

void DongleStatusDisplay::render(const ble::EcoFlowBleSnapshot& bleSnapshot,
                                 const esp_sys::EspHealthSnapshot& health,
                                 const DongleBridgeLink& link) {
  if (!panel_.ready()) {
    return;
  }
  switch (pageIndex_) {
    case 0:
      drawOverview(bleSnapshot, link);
      break;
    case 1:
      drawInputs(bleSnapshot);
      break;
    case 2:
      drawOutputs(bleSnapshot);
      break;
    case 3:
      drawPack(bleSnapshot);
      break;
    case 4:
      drawMeter(bleSnapshot);
      break;
    case 5:
      drawLink(bleSnapshot, link);
      break;
    case 6:
      drawEsp(health);
      break;
    default:
      drawOverview(bleSnapshot, link);
      break;
  }
  panel_.present();
}

}  // namespace ecoflow::display
