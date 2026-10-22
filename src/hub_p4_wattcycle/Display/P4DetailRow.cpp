#include "Display/P4DetailRow.h"

#include "Display/P4OverviewStyle.h"

namespace wattcycle::display {

lv_obj_t* makeDetailRoot(lv_obj_t* parent) {
  lv_obj_t* root = lv_obj_create(parent);
  lv_obj_set_pos(root, 0, kStatusH);
  lv_obj_set_size(root, 480, kMenuY - kStatusH);
  lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(root, 0, 0);
  lv_obj_set_style_pad_all(root, 0, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
  return root;
}

lv_obj_t* makeLabeledRow(lv_obj_t* parent, int y, const char* name) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_set_pos(card, 16, y);
  lv_obj_set_size(card, 448, 64);
  lv_obj_set_style_bg_color(card, lv_color_hex(kCard), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* nameLabel = lv_label_create(card);
  lv_label_set_text(nameLabel, name);
  lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(nameLabel, lv_color_hex(kMuted), 0);
  lv_obj_set_pos(nameLabel, 16, 20);
  lv_obj_t* value = lv_label_create(card);
  lv_label_set_text(value, "--");
  lv_obj_set_style_text_font(value, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(value, lv_color_hex(kInk), 0);
  lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_size(value, 230, 22);
  lv_obj_set_pos(value, 200, 20);
  return value;
}

void setDetailValue(lv_obj_t* label, const char* text, uint32_t color) {
  if (label == nullptr) {
    return;
  }
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
}

}  // namespace wattcycle::display
