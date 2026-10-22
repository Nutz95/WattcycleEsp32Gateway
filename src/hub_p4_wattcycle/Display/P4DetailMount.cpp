#include "Display/P4DetailPages.h"

#include "Display/P4DetailRow.h"
#include "Display/P4OverviewStyle.h"

#include <cstdint>

namespace wattcycle::display {
namespace {

void onResetTap(lv_event_t* event) {
  auto* pages = static_cast<P4DetailPages*>(lv_event_get_user_data(event));
  const auto command = static_cast<uint8_t>(
      reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target(event))));
  if (pages != nullptr) {
    pages->dispatchMeterCommand(command);
  }
}

void mountCommandButton(lv_obj_t* parent, int x, int y, const char* caption, uint8_t command,
                        P4DetailPages* pages) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_size(button, 210, 52);
  lv_obj_set_style_bg_color(button, lv_color_hex(kCard), 0);
  lv_obj_set_user_data(button, reinterpret_cast<void*>(static_cast<uintptr_t>(command)));
  lv_obj_add_event_cb(button, onResetTap, LV_EVENT_CLICKED, pages);
  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, caption);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_center(label);
}

void mountRows(P4TextPage& page, lv_obj_t* parent, const char* const* names, int count) {
  page.root = makeDetailRoot(parent);
  for (int index = 0; index < count && index < kDetailRows; ++index) {
    page.values[index] = makeLabeledRow(page.root, 12 + (index * 76), names[index]);
  }
}

}  // namespace

void P4DetailPages::mount(lv_obj_t* parent) {
  static const char* packNames[] = {"SOC", "Voltage", "Current", "Power", "Capacity", "Health",
                                    "Link", "Security"};
  static const char* solarNames[] = {"Power", "Voltage", "Current", "Energy", "Capacity", "Temp",
                                    "Link", "Security"};
  static const char* ecoNames[] = {"SOC", "Battery", "AC out", "DC out", "USB", "Link", "Security"};
  static const char* tempNames[] = {"MOS", "PCB", "Probe 1", "Probe 2"};
  static const char* alertNames[] = {"Protection", "Fault", "Note"};
  static const char* gateNames[] = {"Hub MAC", "BMS peer", "Eco peer", "Solar peer", "Wi-Fi"};
  static const char* espNames[] = {"This MAC", "IP", "P4 heap", "P4 core 0", "P4 core 1",
                                  "P4 temp", "Uptime", "C6 heap", "SD card", "SD space"};
  static const char* linkNames[] = {"Wi-Fi", "Address", "BMS link", "BMS security", "Solar link",
                                    "Solar security", "EcoFlow link", "EcoFlow security"};
  mountRows(widgets_.wattcycle, parent, packNames, 8);
  mountRows(widgets_.solar, parent, solarNames, 8);
  mountCommandButton(widgets_.solar.root, 16, 620, "Reset Wh", 0x01, this);
  mountCommandButton(widgets_.solar.root, 242, 620, "Reset Ah", 0x02, this);
  lv_obj_add_flag(widgets_.solar.root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(widgets_.solar.root, LV_DIR_VER);
  mountRows(widgets_.eco, parent, ecoNames, 7);
  mountRows(widgets_.temps, parent, tempNames, 4);
  mountRows(widgets_.alerts, parent, alertNames, 3);
  mountRows(widgets_.gateway, parent, gateNames, 5);
  mountRows(widgets_.esp, parent, espNames, 10);
  mountRows(widgets_.link, parent, linkNames, 8);
  lv_obj_add_flag(widgets_.esp.root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(widgets_.esp.root, LV_DIR_VER);
  widgets_.cells.root = makeDetailRoot(parent);
  widgets_.cells.summary = makeLabeledRow(widgets_.cells.root, 12, "Span");
  for (int index = 0; index < kP4CellSlots; ++index) {
    makeCellSlot(widgets_.cells, index, 88 + (index * 58));
  }
}

}  // namespace wattcycle::display
