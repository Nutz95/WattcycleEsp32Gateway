#include "P4AuthTap.h"

#include "Display/P4OverviewStyle.h"

namespace wattcycle::panel {

void P4AuthTap::onClicked(lv_event_t* event) {
  auto* tap = static_cast<P4AuthTap*>(lv_event_get_user_data(event));
  if (tap != nullptr && tap->handler_ != nullptr) {
    tap->handler_(tap->user_);
  }
}

void P4AuthTap::mount(lv_obj_t* parent) {
  bar_ = lv_btn_create(parent);
  lv_obj_set_pos(bar_, 16, 620);
  lv_obj_set_size(bar_, 448, 84);
  lv_obj_set_style_bg_color(bar_, lv_color_hex(display::kAccent), 0);
  lv_obj_set_style_radius(bar_, 14, 0);
  lv_obj_set_style_shadow_width(bar_, 0, 0);
  label_ = lv_label_create(bar_);
  lv_obj_set_style_text_font(label_, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(label_, lv_color_hex(display::kBg), 0);
  lv_label_set_text(label_, "Tap to confirm web account");
  lv_obj_center(label_);
  lv_obj_add_flag(bar_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(bar_, onClicked, LV_EVENT_CLICKED, this);
}

void P4AuthTap::apply(auth::AuthPromptKind kind) {
  if (bar_ == nullptr || kind == shown_) {
    return;
  }
  shown_ = kind;
  if (kind == auth::AuthPromptKind::None) {
    lv_obj_add_flag(bar_, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  const char* text = "Tap to confirm web account";
  if (kind == auth::AuthPromptKind::ConfirmReset) {
    text = "Tap to confirm reset";
  }
  lv_label_set_text(label_, text);
  lv_obj_clear_flag(bar_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(bar_);
}

void P4AuthTap::setHandler(void (*handler)(void* user), void* user) {
  handler_ = handler;
  user_ = user;
}

}  // namespace wattcycle::panel
