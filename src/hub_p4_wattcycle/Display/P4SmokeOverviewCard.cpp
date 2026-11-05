#include "Display/P4SmokeOverviewCard.h"

#include "Display/HubPage.h"
#include "Display/P4OverviewStyle.h"

#include <cstdint>

namespace wattcycle::display {
namespace {

void onSmokeCardTap(lv_event_t* event) {
  auto* navigator = static_cast<IHubPageNavigator*>(lv_event_get_user_data(event));
  if (navigator != nullptr) {
    navigator->showPage(static_cast<uint8_t>(HubPage::Smoke));
  }
}

}  // namespace

void P4SmokeOverviewCard::mount(lv_obj_t* parent, IHubPageNavigator* navigator) {
  card_ = makeCard(parent, kSmokeX, kSmokeY, kSmokeW, kSmokeH);
  const lv_coord_t border = lv_obj_get_style_border_width(card_, 0);
  const lv_coord_t innerW = lv_obj_get_style_width(card_, 0) - (2 * border);
  stampTitle(card_, "Smoke", 8, innerW);
  led_ = lv_obj_create(card_);
  lv_obj_set_size(led_, kSmokeLedSize, kSmokeLedSize);
  lv_obj_set_style_radius(led_, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(led_, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(led_, lv_color_hex(kIdleEdge), 0);
  lv_obj_set_style_border_width(led_, 0, 0);
  lv_obj_set_style_pad_all(led_, 0, 0);
  lv_obj_clear_flag(led_, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(led_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_align(led_, LV_ALIGN_CENTER, 0, 6);
  status_ = makeValueLabel(card_);
  lv_obj_set_width(status_, innerW);
  lv_obj_set_style_text_align(status_, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(status_, "--");
  lv_obj_align(status_, LV_ALIGN_BOTTOM_MID, 0, -8);
  lv_obj_add_flag(card_, LV_OBJ_FLAG_CLICKABLE);
  const uint32_t children = lv_obj_get_child_cnt(card_);
  for (uint32_t child = 0; child < children; ++child) {
    lv_obj_clear_flag(lv_obj_get_child(card_, child), LV_OBJ_FLAG_CLICKABLE);
  }
  lv_obj_add_event_cb(card_, onSmokeCardTap, LV_EVENT_CLICKED, navigator);
}

void P4SmokeOverviewCard::apply(const P4SmokeFields& smoke) {
  if (led_ == nullptr || status_ == nullptr || card_ == nullptr) {
    return;
  }
  uint32_t led = kIdleEdge;
  uint32_t border = kIdleEdge;
  const char* text = "none";
  if (smoke.count == 0) {
    text = "none";
  } else if (smoke.anyAlarm) {
    led = kDanger;
    border = kDanger;
    text = "ALARM";
  } else {
    bool anyOnline = false;
    for (uint8_t i = 0; i < smoke.count; ++i) {
      if (smoke.online[i]) {
        anyOnline = true;
        break;
      }
    }
    if (anyOnline) {
      led = kAccent;
      border = kAccent;
      text = "ok";
    } else {
      led = kSelf;
      border = kIdleEdge;
      text = "stale";
    }
  }
  lv_obj_set_style_bg_color(led_, lv_color_hex(led), 0);
  lv_obj_set_style_border_color(card_, lv_color_hex(border), 0);
  lv_label_set_text(status_, text);
  lv_obj_set_style_text_color(status_, lv_color_hex(smoke.anyAlarm ? kDanger : kMuted), 0);
}

}  // namespace wattcycle::display
