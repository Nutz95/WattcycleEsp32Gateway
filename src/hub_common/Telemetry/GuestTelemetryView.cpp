#include "Telemetry/GuestTelemetryView.h"

namespace wattcycle::telemetry {
namespace {

void clearText(char* text) {
  if (text != nullptr) {
    text[0] = '\0';
  }
}

void dropBridgeIdentity(bool& bridgeValid, EspHealth& health) {
  bridgeValid = false;
  health = {};
}

SolarBridgeTelemetry solarOverview(const SolarBridgeTelemetry& full) {
  SolarBridgeTelemetry brief;
  brief.linkFresh = full.linkFresh;
  brief.sppConnected = full.sppConnected;
  brief.meterValid = full.meterValid;
  brief.espNowEncrypted = full.espNowEncrypted;
  brief.voltageV = full.voltageV;
  brief.currentA = full.currentA;
  brief.powerW = full.powerW;
  brief.receivedAtMs = full.receivedAtMs;
  return brief;
}

}  // namespace

GuestTelemetryView::GuestTelemetryView(const ITelemetryStore& inner, uint32_t mask)
    : inner_(inner), mask_(mask & auth::IGuestAccess::kTabMask) {}

bool GuestTelemetryView::allows(uint32_t bit) const {
  return (mask_ & bit) != 0;
}

bms::BatteryTelemetry GuestTelemetryView::battery() const {
  const bool summary = allows(auth::IGuestAccess::kTabOverview) ||
                       allows(auth::IGuestAccess::kTabDashboard) ||
                       allows(auth::IGuestAccess::kTabWattcycle);
  const bool cells = allows(auth::IGuestAccess::kTabCells);
  const bool temps = allows(auth::IGuestAccess::kTabTemperatures);
  if (!summary && !cells && !temps) {
    return {};
  }
  auto out = inner_.battery();
  if (!summary) {
    out.currentAmps = 0.0f;
    out.moduleVoltage = 0.0f;
    out.remainingCapacityAh = 0.0f;
    out.totalCapacityAh = 0.0f;
    out.designCapacityAh = 0.0f;
    out.cycleNumber = 0;
    out.stateOfChargePercent = 0;
    out.stateOfHealthPercent = 0;
    out.powerWatts = 0.0f;
    out.balanceCurrentAmps = 0.0f;
  }
  if (!cells) {
    out.cellCount = 0;
  }
  if (!temps) {
    out.temperatureCount = 0;
    out.mosTemperatureC = 0.0f;
    out.pcbTemperatureC = 0.0f;
  }
  out.valid = summary || cells || temps;
  return out;
}

bms::ProductInfo GuestTelemetryView::product() const {
  if (!allows(auth::IGuestAccess::kTabWattcycle)) {
    return {};
  }
  auto out = inner_.product();
  clearText(out.serialNumber);
  clearText(out.manufacturerName);
  return out;
}

bms::WarningFlags GuestTelemetryView::warnings() const {
  if (!allows(auth::IGuestAccess::kTabWarnings) && !allows(auth::IGuestAccess::kTabCells)) {
    return {};
  }
  return inner_.warnings();
}

SolarBridgeTelemetry GuestTelemetryView::solar() const {
  const bool full = allows(auth::IGuestAccess::kTabSolar);
  const bool summary =
      full || allows(auth::IGuestAccess::kTabOverview) || allows(auth::IGuestAccess::kTabDashboard);
  if (!summary) {
    return {};
  }
  const SolarBridgeTelemetry out = inner_.solar();
  if (!full) {
    return solarOverview(out);
  }
  SolarBridgeTelemetry detailed = out;
  clearText(detailed.sppTarget);
  clearText(detailed.lastError);
  if (!allows(auth::IGuestAccess::kTabEsp)) {
    dropBridgeIdentity(detailed.bridgeEspValid, detailed.bridgeEsp);
  }
  return detailed;
}

EcoFlowBridgeTelemetry GuestTelemetryView::ecoflow() const {
  const bool full = allows(auth::IGuestAccess::kTabEcoFlow);
  const bool summary =
      full || allows(auth::IGuestAccess::kTabOverview) || allows(auth::IGuestAccess::kTabDashboard);
  if (!summary) {
    return {};
  }
  auto out = inner_.ecoflow();
  clearText(out.lastError);
  if (!allows(auth::IGuestAccess::kTabEsp)) {
    dropBridgeIdentity(out.bridgeEspValid, out.bridgeEsp);
  }
  if (full) {
    return out;
  }
  EcoFlowBridgeTelemetry brief;
  brief.linkFresh = out.linkFresh;
  brief.bleConnected = out.bleConnected;
  brief.meterValid = out.meterValid;
  brief.socPercent = out.socPercent;
  brief.acOutputW = out.acOutputW;
  brief.acInputW = out.acInputW;
  brief.dcOutputW = out.dcOutputW;
  brief.usbOutputW = out.usbOutputW;
  brief.solarInputW = out.solarInputW;
  brief.acOutputOn = out.acOutputOn;
  brief.dcOutputOn = out.dcOutputOn;
  brief.usbOutputOn = out.usbOutputOn;
  return brief;
}

GatewayStatus GuestTelemetryView::status() const {
  const bool links = allows(auth::IGuestAccess::kTabOverview) ||
                     allows(auth::IGuestAccess::kTabDashboard) ||
                     allows(auth::IGuestAccess::kTabGateway) ||
                     allows(auth::IGuestAccess::kTabWattcycle) ||
                     allows(auth::IGuestAccess::kTabSolar) ||
                     allows(auth::IGuestAccess::kTabEcoFlow);
  if (!links) {
    return {};
  }
  auto out = inner_.status();
  clearText(out.wifiIp);
  clearText(out.bleAddress);
  clearText(out.lastError);
  out.webPort = 0;
  return out;
}

EspHealth GuestTelemetryView::espHealth() const {
  if (!allows(auth::IGuestAccess::kTabEsp)) {
    return {};
  }
  return inner_.espHealth();
}

EspHealth GuestTelemetryView::bmsBridgeHealth() const {
  if (!allows(auth::IGuestAccess::kTabEsp)) {
    return {};
  }
  return inner_.bmsBridgeHealth();
}

bool GuestTelemetryView::bmsBridgeHealthValid() const {
  return allows(auth::IGuestAccess::kTabEsp) && inner_.bmsBridgeHealthValid();
}

}  // namespace wattcycle::telemetry
