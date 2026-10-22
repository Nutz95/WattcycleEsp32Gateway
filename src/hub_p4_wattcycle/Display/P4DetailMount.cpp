#include "Display/P4DetailPages.h"

#include "Display/P4CellWidgets.h"
#include "Display/P4DetailRow.h"

namespace wattcycle::display {
namespace {

void mountRows(P4TextPage& page, lv_obj_t* parent, const char* const* names, int count) {
  page.root = makeDetailRoot(parent);
  for (int index = 0; index < count && index < kDetailRows; ++index) {
    page.values[index] = makeLabeledRow(page.root, 12 + (index * 76), names[index]);
  }
}

}  // namespace

void P4DetailPages::mount(lv_obj_t* parent) {
  static const char* packNames[] = {"SOC", "Voltage", "Current", "Power", "Capacity", "Health"};
  static const char* solarNames[] = {"Power", "Voltage", "Link"};
  static const char* ecoNames[] = {"SOC", "Battery", "AC out", "DC out", "USB", "Link"};
  static const char* tempNames[] = {"MOS", "PCB", "Probe 1", "Probe 2"};
  static const char* alertNames[] = {"Protection", "Fault", "Note"};
  static const char* gateNames[] = {"Wi-Fi", "BMS", "EcoFlow", "Solar"};
  static const char* espNames[] = {"Heap", "Core 0", "Core 1"};
  mountRows(widgets_.wattcycle, parent, packNames, 6);
  mountRows(widgets_.solar, parent, solarNames, 3);
  mountRows(widgets_.eco, parent, ecoNames, 6);
  mountRows(widgets_.temps, parent, tempNames, 4);
  mountRows(widgets_.alerts, parent, alertNames, 3);
  mountRows(widgets_.gateway, parent, gateNames, 4);
  mountRows(widgets_.esp, parent, espNames, 3);
  widgets_.cells.root = makeDetailRoot(parent);
  widgets_.cells.summary = makeLabeledRow(widgets_.cells.root, 12, "Span");
  for (int index = 0; index < kP4CellSlots; ++index) {
    makeCellSlot(widgets_.cells, index, 88 + (index * 58));
  }
}

}  // namespace wattcycle::display
