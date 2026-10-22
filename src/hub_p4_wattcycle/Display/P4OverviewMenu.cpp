#include "Display/P4OverviewView.h"

#include "Display/P4MenuThermometer.h"

#include <cstdint>
#include <cstdio>

namespace wattcycle::display {
namespace {

const char* menuCaption(uint8_t index) {
  static const char* captions[kHubPageCount] = {
      "Overview", "Wattcycle", "Solar", "EcoFlow", "Cells", "Temps", "Alerts", "Gateway", "Esp"};
  if (index >= kHubPageCount) {
    return "";
  }
  return captions[index];
}

const char* menuSymbol(uint8_t index) {
  static const char* symbols[kHubPageCount] = {
      LV_SYMBOL_HOME, LV_SYMBOL_BATTERY_FULL, LV_SYMBOL_CHARGE, LV_SYMBOL_POWER, LV_SYMBOL_BATTERY_3,
      LV_SYMBOL_TINT, LV_SYMBOL_BELL,         LV_SYMBOL_SETTINGS, LV_SYMBOL_USB};
  if (index >= kHubPageCount) {
    return "";
  }
  return symbols[index];
}

void onMenuTap(lv_event_t* event) {
  auto* view = static_cast<P4OverviewView*>(lv_event_get_user_data(event));
  const auto index = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target(event)));
  if (view != nullptr) {
    view->showPage(static_cast<uint8_t>(index));
  }
}

void paintMenuGlyph(lv_obj_t* icon, uint32_t color) {
  const uint32_t children = lv_obj_get_child_cnt(icon);
  if (children == 0) {
    lv_obj_set_style_text_color(icon, lv_color_hex(color), 0);
    return;
  }
  for (uint32_t child = 0; child < children; ++child) {
    lv_obj_set_style_bg_color(lv_obj_get_child(icon, child), lv_color_hex(color), 0);
  }
}

lv_obj_t* makeMenuIcon(lv_obj_t* button, uint8_t index) {
  if (index == static_cast<uint8_t>(HubPage::Temps)) {
    lv_obj_t* thermo = makeMenuThermometer(button);
    lv_obj_align(thermo, LV_ALIGN_TOP_MID, 0, 4);
    return thermo;
  }
  lv_obj_t* icon = lv_label_create(button);
  lv_label_set_text(icon, menuSymbol(index));
  lv_obj_set_style_text_font(icon, &lv_font_montserrat_20, 0);
  lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 8);
  return icon;
}

}  // namespace

void P4OverviewView::mountMenu(lv_obj_t* parent) {
  menu_ = lv_obj_create(parent);
  lv_obj_set_pos(menu_, 0, kMenuY);
  lv_obj_set_size(menu_, 480, kMenuH);
  lv_obj_set_style_bg_color(menu_, lv_color_hex(kCard), 0);
  lv_obj_set_style_bg_opa(menu_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(menu_, 0, 0);
  lv_obj_set_style_radius(menu_, 0, 0);
  lv_obj_set_style_pad_all(menu_, 0, 0);
  lv_obj_set_scroll_dir(menu_, LV_DIR_HOR);
  lv_obj_set_scrollbar_mode(menu_, LV_SCROLLBAR_MODE_OFF);
  for (uint8_t index = 0; index < kHubPageCount; ++index) {
    lv_obj_t* button = lv_btn_create(menu_);
    lv_obj_set_pos(button, 8 + (static_cast<int>(index) * 104), 6);
    lv_obj_set_size(button, 96, 68);
    lv_obj_set_style_shadow_width(button, 0, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_set_user_data(button, reinterpret_cast<void*>(static_cast<uintptr_t>(index)));
    lv_obj_add_event_cb(button, onMenuTap, LV_EVENT_CLICKED, this);
    lv_obj_t* caption = lv_label_create(button);
    lv_label_set_text(caption, menuCaption(index));
    lv_obj_set_width(caption, 96);
    lv_obj_set_style_text_font(caption, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(caption, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(caption, LV_ALIGN_BOTTOM_MID, 0, -4);
    menuBtn_[index] = button;
    menuIcon_[index] = makeMenuIcon(button, index);
  }
}

void P4OverviewView::styleMenu(uint8_t selected) {
  for (uint8_t index = 0; index < kHubPageCount; ++index) {
    const bool on = index == selected;
    lv_obj_set_style_bg_color(menuBtn_[index], lv_color_hex(on ? 0x243044 : kBg), 0);
    lv_obj_set_style_border_width(menuBtn_[index], 2, 0);
    lv_obj_set_style_border_color(menuBtn_[index], lv_color_hex(on ? kAccent : kIdleEdge), 0);
    paintMenuGlyph(menuIcon_[index], on ? kAccent : kMuted);
  }
}

void P4OverviewView::showPage(uint8_t index) {
  if (index >= kHubPageCount) {
    return;
  }
  page_ = index;
  const bool overview = index == static_cast<uint8_t>(HubPage::Overview);
  if (overview) {
    lv_obj_clear_flag(flowRoot_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(flowRoot_, LV_OBJ_FLAG_HIDDEN);
  }
  lv_obj_add_flag(placeholder_, LV_OBJ_FLAG_HIDDEN);
  styleMenu(index);
  if (pageHandler_ != nullptr) {
    pageHandler_(index, pageUser_);
  }
}

void P4OverviewView::setPageHandler(void (*handler)(uint8_t index, void* user), void* user) {
  pageHandler_ = handler;
  pageUser_ = user;
}

}  // namespace wattcycle::display
