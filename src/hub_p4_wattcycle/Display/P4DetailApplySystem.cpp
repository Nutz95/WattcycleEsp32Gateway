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
  setDetailValue(page.values[0], macOrDash(fields.hubMac), kInk);
  setDetailValue(page.values[1], macOrDash(fields.bmsMac), fields.bmsUp ? kAccent : kDanger);
  setDetailValue(page.values[2], macOrDash(fields.ecoMac), fields.ecoUp ? kAccent : kDanger);
  setDetailValue(page.values[3], macOrDash(fields.solarMac), fields.solarUp ? kAccent : kDanger);
  setDetailValue(page.values[4], fields.wifiUp ? "up" : "down", fields.wifiUp ? kAccent : kDanger);
  setDetailValue(page.values[5], fields.guestOn ? "on" : "off", fields.guestOn ? kAccent : kMuted);
}

void formatUptime(char* line, size_t capacity, int seconds) {
  if (seconds < 0) {
    seconds = 0;
  }
  const int minutes = seconds / 60;
  if (minutes < 60) {
    std::snprintf(line, capacity, "%d min", minutes);
    return;
  }
  std::snprintf(line, capacity, "%dh %02d", minutes / 60, minutes % 60);
}

void applyEsp(P4TextPage& page, const P4EspFields& fields) {
  char line[24];
  setDetailValue(page.values[0], macOrDash(fields.deviceMac), kInk);
  setDetailValue(page.values[1], fields.staIp[0] != '\0' ? fields.staIp : "--", kInk);
  std::snprintf(line, sizeof(line), "%d kB", fields.heapKb);
  setDetailValue(page.values[2], line, kInk);
  std::snprintf(line, sizeof(line), "%d%%", fields.cpu0);
  setDetailValue(page.values[3], line, kInk);
  std::snprintf(line, sizeof(line), "%d%%", fields.cpu1);
  setDetailValue(page.values[4], line, kInk);
  std::snprintf(line, sizeof(line), "%d C", fields.tempC);
  setDetailValue(page.values[5], line, kInk);
  formatUptime(line, sizeof(line), fields.uptimeSec);
  setDetailValue(page.values[6], line, kInk);
  if (fields.c6HeapKb < 0) {
    setDetailValue(page.values[7], "--", kMuted);
  } else {
    std::snprintf(line, sizeof(line), "%d kB", fields.c6HeapKb);
    setDetailValue(page.values[7], line, kInk);
  }
  setDetailValue(page.values[8], fields.sdReady ? "ready" : "not seen",
                 fields.sdReady ? kAccent : kDanger);
  if (!fields.sdReady || fields.sdTotalMb <= 0) {
    setDetailValue(page.values[9], "--", kMuted);
    return;
  }
  std::snprintf(line, sizeof(line), "%d / %d MB", fields.sdUsedMb, fields.sdTotalMb);
  setDetailValue(page.values[9], line, kInk);
}

void applyPlugs(P4TextPage& page, const P4PlugsFields& fields) {
  char line[40];
  if (fields.count == 0) {
    setDetailValue(page.values[0], "authorize on web", kMuted);
    for (int index = 1; index < 9; ++index) {
      setDetailValue(page.values[index], "--", kMuted);
    }
    return;
  }
  std::snprintf(line, sizeof(line), "%.0f W", fields.totalWatts);
  setDetailValue(page.values[0], line, kInk);
  for (int index = 0; index < 8; ++index) {
    if (index >= fields.count) {
      setDetailValue(page.values[index + 1], "--", kMuted);
      continue;
    }
    const char* name = fields.names[index][0] != '\0' ? fields.names[index] : "plug";
    if (!fields.online[index]) {
      std::snprintf(line, sizeof(line), "%s offline", name);
      setDetailValue(page.values[index + 1], line, kDanger);
      continue;
    }
    std::snprintf(line, sizeof(line), "%s %.0fW %.0fWh", name, fields.watts[index],
                  fields.whToday[index]);
    setDetailValue(page.values[index + 1], line, fields.output[index] ? kAccent : kInk);
  }
}

void applyLink(P4TextPage& page, const P4PanelBoard& board) {
  const P4GatewayFields& fields = board.gateway;
  const bool wifiUp = fields.wifiUp;
  const char* note = fields.wifiNote[0] != '\0' ? fields.wifiNote : (wifiUp ? "up" : "down");
  setDetailValue(page.values[0], note, wifiUp ? kAccent : kDanger);
  setDetailValue(page.values[1], board.esp.staIp[0] != '\0' ? board.esp.staIp : "--", kInk);
  setDetailValue(page.values[2], fields.bmsUp ? "live" : "no link", fields.bmsUp ? kAccent : kDanger);
  setSecurityValue(page.values[3], board.wattcycle.linkUp, board.wattcycle.secured);
  setDetailValue(page.values[4], fields.solarUp ? "live" : "no link", fields.solarUp ? kAccent : kDanger);
  setSecurityValue(page.values[5], board.solar.linkUp, board.solar.secured);
  setDetailValue(page.values[6], fields.ecoUp ? "live" : "no link", fields.ecoUp ? kAccent : kDanger);
  setSecurityValue(page.values[7], board.eco.linkUp, board.eco.secured);
}

}  // namespace

void applySystemPages(P4DetailWidgets& widgets, const P4PanelBoard& board) {
  applyTemps(widgets.temps, board.temps);
  applyAlerts(widgets.alerts, board.alerts);
  applyGateway(widgets.gateway, board.gateway);
  applyEsp(widgets.esp, board.esp);
  applyPlugs(widgets.plugs, board.plugs);
  applyLink(widgets.link, board);
}

}  // namespace wattcycle::display
