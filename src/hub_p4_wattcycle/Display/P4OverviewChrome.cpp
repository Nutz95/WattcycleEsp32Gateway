#include "Display/P4OverviewView.h"

#include <cstdio>
#include <ctime>

namespace wattcycle::display {
namespace {

void paintIcon(lv_obj_t* icon, bool up) {
  lv_obj_set_style_text_color(icon, lv_color_hex(up ? kAccent : kDanger), 0);
}

lv_obj_t* makeBar(lv_obj_t* parent, int y, int height) {
  lv_obj_t* bar = lv_obj_create(parent);
  lv_obj_set_pos(bar, 0, y);
  lv_obj_set_size(bar, 480, height);
  lv_obj_set_style_bg_color(bar, lv_color_hex(kCard), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 0, 0);
  lv_obj_set_style_pad_all(bar, 0, 0);
  lv_obj_set_style_shadow_width(bar, 0, 0);
  return bar;
}

}  // namespace

void P4OverviewView::mountStatus(lv_obj_t* parent) {
  lv_obj_t* bar = makeBar(parent, 0, kStatusH);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  wifiIcon_ = lv_label_create(bar);
  lv_label_set_text(wifiIcon_, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_font(wifiIcon_, &lv_font_montserrat_20, 0);
  lv_obj_align(wifiIcon_, LV_ALIGN_LEFT_MID, 16, 0);
  espIcon_ = lv_label_create(bar);
  lv_label_set_text(espIcon_, LV_SYMBOL_BLUETOOTH);
  lv_obj_set_style_text_font(espIcon_, &lv_font_montserrat_20, 0);
  lv_obj_align(espIcon_, LV_ALIGN_LEFT_MID, 52, 0);
  clock_ = lv_label_create(bar);
  lv_obj_set_style_text_font(clock_, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(clock_, lv_color_hex(kInk), 0);
  lv_label_set_text(clock_, "--:--");
  lv_obj_align(clock_, LV_ALIGN_RIGHT_MID, -16, -10);
  date_ = lv_label_create(bar);
  lv_obj_set_style_text_font(date_, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(date_, lv_color_hex(kMuted), 0);
  lv_label_set_text(date_, "--/--");
  lv_obj_align(date_, LV_ALIGN_RIGHT_MID, -16, 12);
  placeholder_ = lv_label_create(parent);
  lv_obj_set_width(placeholder_, 420);
  lv_label_set_long_mode(placeholder_, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(placeholder_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(placeholder_, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(placeholder_, lv_color_hex(kInk), 0);
  lv_obj_align(placeholder_, LV_ALIGN_CENTER, 0, -20);
  lv_obj_add_flag(placeholder_, LV_OBJ_FLAG_HIDDEN);
}

void P4OverviewView::setLinks(bool wifiUp, bool espUp) {
  wifiUp_ = wifiUp;
  espUp_ = espUp;
  paintIcon(wifiIcon_, wifiUp_);
  paintIcon(espIcon_, espUp_);
}

void P4OverviewView::refreshClock() {
  if (clock_ == nullptr || date_ == nullptr) {
    return;
  }
  const time_t now = time(nullptr);
  tm local = {};
  localtime_r(&now, &local);
  const bool known = local.tm_year >= (2024 - 1900);
  const int stamp = known ? ((local.tm_hour * 60) + local.tm_min) : -1;
  if (stamp == clockMinute_) {
    return;
  }
  clockMinute_ = stamp;
  char clockText[8];
  char dateText[8];
  if (!known) {
    std::snprintf(clockText, sizeof(clockText), "--:--");
    std::snprintf(dateText, sizeof(dateText), "--/--");
  } else {
    std::snprintf(clockText, sizeof(clockText), "%02d:%02d", local.tm_hour, local.tm_min);
    std::snprintf(dateText, sizeof(dateText), "%02d/%02d", local.tm_mday, local.tm_mon + 1);
  }
  lv_label_set_text(clock_, clockText);
  lv_label_set_text(date_, dateText);
}

}  // namespace wattcycle::display
