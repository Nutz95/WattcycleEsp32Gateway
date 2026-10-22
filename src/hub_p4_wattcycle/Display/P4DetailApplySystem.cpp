#include "Display/P4DetailPages.h"

#include "Display/P4DetailRow.h"
#include "Display/P4OverviewStyle.h"

#include <cstdio>

namespace wattcycle::display {
namespace {

void applyTemps(P4TextPage& page, const P4TempsFields& fields) {
  char line[16];
  if (fields.valid) {
    std::snprintf(line, sizeof(line), "%d C", fields.mosC);
  } else {
    std::snprintf(line, sizeof(line), "--");
  }
  setDetailValue(page.values[0], line, kInk);
  if (fields.valid) {
    std::snprintf(line, sizeof(line), "%d C", fields.pcbC);
  } else {
    std::snprintf(line, sizeof(line), "--");
  }
  setDetailValue(page.values[1], line, kInk);
  if (fields.probeCount > 0) {
    std::snprintf(line, sizeof(line), "%d C", fields.probeC[0]);
  } else {
    std::snprintf(line, sizeof(line), "--");
  }
  setDetailValue(page.values[2], line, kInk);
  if (fields.probeCount > 1) {
    std::snprintf(line, sizeof(line), "%d C", fields.probeC[1]);
  } else {
    std::snprintf(line, sizeof(line), "--");
  }
  setDetailValue(page.values[3], line, kInk);
}

void applyAlerts(P4TextPage& page, const P4AlertsFields& fields) {
  setDetailValue(page.values[0], fields.protection ? "on" : "off", fields.protection ? kDanger : kAccent);
  setDetailValue(page.values[1], fields.fault ? "on" : "off", fields.fault ? kDanger : kAccent);
  setDetailValue(page.values[2], fields.note[0] != '\0' ? fields.note : "--", kMuted);
}

void applyGateway(P4TextPage& page, const P4GatewayFields& fields) {
  setDetailValue(page.values[0], fields.wifiUp ? "up" : "down", fields.wifiUp ? kAccent : kDanger);
  setDetailValue(page.values[1], fields.bmsUp ? "live" : "no link", fields.bmsUp ? kAccent : kDanger);
  setDetailValue(page.values[2], fields.ecoUp ? "live" : "no link", fields.ecoUp ? kAccent : kDanger);
  setDetailValue(page.values[3], fields.solarUp ? "live" : "no link", fields.solarUp ? kAccent : kDanger);
}

void applyEsp(P4TextPage& page, const P4EspFields& fields) {
  char line[16];
  std::snprintf(line, sizeof(line), "%d kB", fields.heapKb);
  setDetailValue(page.values[0], line, kInk);
  std::snprintf(line, sizeof(line), "%d%%", fields.cpu0);
  setDetailValue(page.values[1], line, kInk);
  std::snprintf(line, sizeof(line), "%d%%", fields.cpu1);
  setDetailValue(page.values[2], line, kInk);
}

}  // namespace

void applySystemPages(P4DetailWidgets& widgets, const P4PanelBoard& board) {
  applyTemps(widgets.temps, board.temps);
  applyAlerts(widgets.alerts, board.alerts);
  applyGateway(widgets.gateway, board.gateway);
  applyEsp(widgets.esp, board.esp);
}

}  // namespace wattcycle::display
