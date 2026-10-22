#include "Display/P4OverviewView.h"

#include <cstdint>
#include <cstdio>
#include <ctime>

namespace wattcycle::display {
namespace {

void paintIcon(lv_obj_t* icon, bool up) {
  lv_obj_set_style_text_color(icon, lv_color_hex(up ? kAccent : kDanger), 0);
}

void onStatusTap(lv_event_t* event) {
  auto* view = static_cast<P4OverviewView*>(lv_event_get_user_data(event));
  const auto page = reinterpret_cast<uintptr_t>(lv_obj_get_user_data(lv_event_get_target(event)));
  if (view != nullptr) {
    view->showPage(static_cast<uint8_t>(page));
  }
}

void armStatusIcon(P4OverviewView* view, lv_obj_t* icon, HubPage page) {
  lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_user_data(icon, reinterpret_cast<void*>(static_cast<uintptr_t>(page)));
  lv_obj_add_event_cb(icon, onStatusTap, LV_EVENT_CLICKED, view);
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
  armStatusIcon(this, wifiIcon_, HubPage::Link);
  espIcon_ = lv_label_create(bar);
  lv_label_set_text(espIcon_, LV_SYMBOL_BLUETOOTH);
  lv_obj_set_style_text_font(espIcon_, &lv_font_montserrat_20, 0);
  lv_obj_align(espIcon_, LV_ALIGN_LEFT_MID, 52, 0);
  armStatusIcon(this, espIcon_, HubPage::Link);
  sdIcon_ = lv_label_create(bar);
  lv_label_set_text(sdIcon_, LV_SYMBOL_SD_CARD);
  lv_obj_set_style_text_font(sdIcon_, &lv_font_montserrat_20, 0);
  lv_obj_align(sdIcon_, LV_ALIGN_LEFT_MID, 88, 0);
  armStatusIcon(this, sdIcon_, HubPage::Esp);
  sdWarn_ = lv_label_create(bar);
  lv_label_set_text(sdWarn_, LV_SYMBOL_WARNING);
  lv_obj_set_style_text_font(sdWarn_, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(sdWarn_, lv_color_hex(kDanger), 0);
  lv_obj_align(sdWarn_, LV_ALIGN_LEFT_MID, 112, -8);
  lv_obj_add_flag(sdWarn_, LV_OBJ_FLAG_HIDDEN);
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
  setStorage(false, 0, 0);
}

void P4OverviewView::setLinks(bool wifiUp, bool espUp) {
  wifiUp_ = wifiUp;
  espUp_ = espUp;
  paintIcon(wifiIcon_, wifiUp_);
  paintIcon(espIcon_, espUp_);
}

void P4OverviewView::setStorage(bool ready, int usedMb, int totalMb) {
  if (sdIcon_ == nullptr) {
    return;
  }
  uint32_t color = kDanger;
  if (ready && totalMb > usedMb) {
    const int freeMb = totalMb - usedMb;
    const int freePct = (freeMb * 100) / totalMb;
    color = freePct < 10 ? kDanger : (freePct < 25 ? 0xF0B429 : kAccent);
  }
  lv_obj_set_style_text_color(sdIcon_, lv_color_hex(color), 0);
  if (sdWarn_ == nullptr) {
    return;
  }
  if (ready) {
    lv_obj_add_flag(sdWarn_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_flag(sdWarn_, LV_OBJ_FLAG_HIDDEN);
  }
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
