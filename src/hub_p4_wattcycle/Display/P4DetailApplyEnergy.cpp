#include "Display/P4DetailPages.h"

#include "Display/P4DetailRow.h"
#include "Display/P4OverviewStyle.h"

#include <cstdio>

namespace wattcycle::display {
namespace {

void formatMilli(char* line, int size, int milli, const char* unit) {
  int absMilli = milli < 0 ? -milli : milli;
  const char* sign = milli < 0 ? "-" : "";
  std::snprintf(line, static_cast<size_t>(size), "%s%d.%02d %s", sign, absMilli / 1000,
                (absMilli % 1000) / 10, unit);
}

void applyWattcycle(P4TextPage& page, const P4WattcycleFields& fields) {
  char line[32];
  const uint32_t link = fields.linkUp ? kAccent : kDanger;
  if (fields.valid) {
    std::snprintf(line, sizeof(line), "%d%%", fields.soc);
  } else {
    std::snprintf(line, sizeof(line), "--%%");
  }
  setDetailValue(page.values[0], line, link);
  formatMilli(line, sizeof(line), fields.voltsMv, "V");
  setDetailValue(page.values[1], fields.valid ? line : "--", kInk);
  formatMilli(line, sizeof(line), fields.ampsMa, "A");
  setDetailValue(page.values[2], fields.valid ? line : "--", fields.ampsMa < 0 ? kDanger : kAccent);
  std::snprintf(line, sizeof(line), "%d W", fields.watts);
  setDetailValue(page.values[3], fields.valid ? line : "--", fields.watts < 0 ? kDanger : kAccent);
  std::snprintf(line, sizeof(line), "%d.%d / %d.%d Ah", fields.remainAhTimes10 / 10,
                fields.remainAhTimes10 % 10, fields.totalAhTimes10 / 10, fields.totalAhTimes10 % 10);
  setDetailValue(page.values[4], fields.valid ? line : "--", kInk);
  std::snprintf(line, sizeof(line), "%d%%  %d cyc", fields.soh, fields.cycles);
  setDetailValue(page.values[5], fields.valid ? line : "--", kInk);
  setDetailValue(page.values[6], fields.linkUp ? "live" : "no link", link);
  setSecurityValue(page.values[7], fields.linkUp, fields.secured);
}

void applySolar(P4TextPage& page, const P4SolarFields& fields) {
  char line[32];
  std::snprintf(line, sizeof(line), "%d W", fields.watts);
  setDetailValue(page.values[0], fields.linkUp ? line : "--", fields.watts > 0 ? kAccent : kMuted);
  formatMilli(line, sizeof(line), fields.voltsMv, "V");
  setDetailValue(page.values[1], fields.linkUp ? line : "--", kInk);
  setDetailValue(page.values[2], fields.linkUp ? "live" : "no link", fields.linkUp ? kAccent : kDanger);
  setSecurityValue(page.values[3], fields.linkUp, fields.secured);
}

void applyEco(P4TextPage& page, const P4EcoFields& fields) {
  char line[32];
  const uint32_t link = fields.linkUp ? kAccent : kDanger;
  if (fields.valid) {
    std::snprintf(line, sizeof(line), "%d%%", fields.soc);
  } else {
    std::snprintf(line, sizeof(line), "--%%");
  }
  setDetailValue(page.values[0], line, link);
  std::snprintf(line, sizeof(line), "%+d W", fields.batteryW);
  setDetailValue(page.values[1], fields.valid ? line : "--", fields.batteryW < 0 ? kDanger : kAccent);
  if (fields.acOn) {
    std::snprintf(line, sizeof(line), "%d W", fields.acW);
  } else {
    std::snprintf(line, sizeof(line), "off");
  }
  setDetailValue(page.values[2], line, fields.acOn ? kAccent : kMuted);
  if (fields.dcOn) {
    std::snprintf(line, sizeof(line), "%d W", fields.dcW);
  } else {
    std::snprintf(line, sizeof(line), "off");
  }
  setDetailValue(page.values[3], line, fields.dcOn ? kAccent : kMuted);
  if (fields.usbOn) {
    std::snprintf(line, sizeof(line), "%d W", fields.usbW);
  } else {
    std::snprintf(line, sizeof(line), "off");
  }
  setDetailValue(page.values[4], line, fields.usbOn ? kAccent : kMuted);
  setDetailValue(page.values[5], fields.linkUp ? "live" : "no link", link);
  setSecurityValue(page.values[6], fields.linkUp, fields.secured);
}

}  // namespace

void applyEnergyPages(P4DetailWidgets& widgets, const P4PanelBoard& board) {
  applyWattcycle(widgets.wattcycle, board.wattcycle);
  applySolar(widgets.solar, board.solar);
  applyEco(widgets.eco, board.eco);
  int lo = 0;
  int hi = 0;
  if (board.cells.count > 0) {
    lo = board.cells.voltsMv[0];
    hi = lo;
  }
  for (int index = 1; index < board.cells.count && index < kP4CellSlots; ++index) {
    if (board.cells.voltsMv[index] < lo) {
      lo = board.cells.voltsMv[index];
    }
    if (board.cells.voltsMv[index] > hi) {
      hi = board.cells.voltsMv[index];
    }
  }
  char line[32];
  if (board.cells.count > 0) {
    std::snprintf(line, sizeof(line), "%d mV", hi - lo);
  } else {
    std::snprintf(line, sizeof(line), "--");
  }
  setDetailValue(widgets.cells.summary, line, kInk);
  for (int index = 0; index < kP4CellSlots; ++index) {
    const bool live = index < board.cells.count;
    const int volts = board.cells.voltsMv[index];
    const bool spread = hi > lo;
    setCellSlot(widgets.cells, index, volts, board.cells.balancing[index], spread && volts == lo,
                spread && volts == hi, live);
  }
}

}  // namespace wattcycle::display
