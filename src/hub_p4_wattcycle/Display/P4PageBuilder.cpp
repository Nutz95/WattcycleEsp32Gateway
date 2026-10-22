#include "Display/P4PageBuilder.h"

#include "Bms/Protocol/ProtocolConstants.h"

#include <cstdio>

namespace wattcycle::display {
namespace {

const char* linkWord(bool fresh) {
  return fresh ? "live" : "no link";
}

void copyTitle(P4PageModel& model, HubPage page) {
  std::snprintf(model.title, sizeof(model.title), "%s", hubPageTitle(page));
}

void cellSpanMv(const bms::BatteryTelemetry& battery, int& minMv, int& maxMv) {
  minMv = 0;
  maxMv = 0;
  if (battery.cellCount == 0) {
    return;
  }
  const uint8_t count = battery.cellCount < bms::kMaxCellCount ? battery.cellCount
                                                               : bms::kMaxCellCount;
  float lo = battery.cellVoltages[0];
  float hi = lo;
  for (uint8_t i = 1; i < count; ++i) {
    const float volts = battery.cellVoltages[i];
    if (volts < lo) {
      lo = volts;
    }
    if (volts > hi) {
      hi = volts;
    }
  }
  minMv = static_cast<int>(lo * 1000.0f);
  maxMv = static_cast<int>(hi * 1000.0f);
}

void fillWattcycle(P4PageModel& model, const telemetry::ITelemetryStore& store) {
  const auto battery = store.battery();
  std::snprintf(model.line1, sizeof(model.line1), "%u%%  %.2fV", battery.stateOfChargePercent,
                battery.moduleVoltage);
  std::snprintf(model.line2, sizeof(model.line2), "%.2fA  %.0fW", battery.currentAmps,
                battery.powerWatts);
}

void fillSolar(P4PageModel& model, const telemetry::ITelemetryStore& store) {
  const auto solar = store.solar();
  std::snprintf(model.line1, sizeof(model.line1), "%.0fW  %.1fV", solar.powerW, solar.voltageV);
  std::snprintf(model.line2, sizeof(model.line2), "%s", linkWord(solar.linkFresh));
}

void fillEcoFlow(P4PageModel& model, const telemetry::ITelemetryStore& store) {
  const auto eco = store.ecoflow();
  std::snprintf(model.line1, sizeof(model.line1), "%u%%  batt %dW", eco.socPercent, eco.bmsPowerW);
  std::snprintf(model.line2, sizeof(model.line2), "AC out %dW", eco.acOutputW);
  std::snprintf(model.line3, sizeof(model.line3), "%s", linkWord(eco.linkFresh));
}

void fillCells(P4PageModel& model, const telemetry::ITelemetryStore& store) {
  const auto battery = store.battery();
  int minMv = 0;
  int maxMv = 0;
  cellSpanMv(battery, minMv, maxMv);
  std::snprintf(model.line1, sizeof(model.line1), "%u cells", battery.cellCount);
  std::snprintf(model.line2, sizeof(model.line2), "%d-%d mV", minMv, maxMv);
}

void fillTemps(P4PageModel& model, const telemetry::ITelemetryStore& store) {
  const auto battery = store.battery();
  std::snprintf(model.line1, sizeof(model.line1), "MOS %.0fC", battery.mosTemperatureC);
  std::snprintf(model.line2, sizeof(model.line2), "PCB %.0fC", battery.pcbTemperatureC);
}

void fillAlerts(P4PageModel& model, const telemetry::ITelemetryStore& store) {
  const auto warnings = store.warnings();
  std::snprintf(model.line1, sizeof(model.line1), "protection %s",
                warnings.hasActiveProtection ? "on" : "off");
  std::snprintf(model.line2, sizeof(model.line2), "fault %s", warnings.hasFault ? "on" : "off");
}

void fillGateway(P4PageModel& model, const telemetry::ITelemetryStore& store) {
  const auto status = store.status();
  std::snprintf(model.line1, sizeof(model.line1), "Wi-Fi %s", status.wifiConnected ? "up" : "down");
  std::snprintf(model.line2, sizeof(model.line2), "BMS %s",
                status.telemetryFresh ? "live" : "no link");
}

void fillEsp(P4PageModel& model, const telemetry::ITelemetryStore& store) {
  const auto health = store.espHealth();
  std::snprintf(model.line1, sizeof(model.line1), "heap %lu kB",
                static_cast<unsigned long>(health.freeHeapBytes / 1024u));
  std::snprintf(model.line2, sizeof(model.line2), "cpu %u/%u", health.cpuCore0Percent,
                health.cpuCore1Percent);
}

}  // namespace

const char* hubPageTitle(HubPage page) {
  switch (page) {
    case HubPage::Overview:
      return "Overview";
    case HubPage::Wattcycle:
      return "Wattcycle";
    case HubPage::Solar:
      return "Solar";
    case HubPage::EcoFlow:
      return "EcoFlow";
    case HubPage::Cells:
      return "Cells";
    case HubPage::Temps:
      return "Temps";
    case HubPage::Alerts:
      return "Alerts";
    case HubPage::Gateway:
      return "Gateway";
    case HubPage::Esp:
      return "ESP";
    case HubPage::Camera:
      return "Camera";
    case HubPage::Link:
      return "Link";
  }
  return "";
}

P4PageModel buildP4Page(HubPage page, const telemetry::ITelemetryStore& store) {
  P4PageModel model;
  model.page = page;
  copyTitle(model, page);
  switch (page) {
    case HubPage::Wattcycle:
      fillWattcycle(model, store);
      break;
    case HubPage::Solar:
      fillSolar(model, store);
      break;
    case HubPage::EcoFlow:
      fillEcoFlow(model, store);
      break;
    case HubPage::Cells:
      fillCells(model, store);
      break;
    case HubPage::Temps:
      fillTemps(model, store);
      break;
    case HubPage::Alerts:
      fillAlerts(model, store);
      break;
    case HubPage::Gateway:
      fillGateway(model, store);
      break;
    case HubPage::Esp:
      fillEsp(model, store);
      break;
    case HubPage::Overview:
    case HubPage::Camera:
    case HubPage::Link:
      break;
  }
  return model;
}

}  // namespace wattcycle::display
