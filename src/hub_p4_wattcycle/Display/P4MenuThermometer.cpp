#include "Display/P4MenuThermometer.h"

#include "Display/P4OverviewStyle.h"

namespace wattcycle::display {
namespace {

void plain(lv_obj_t* obj) {
  lv_obj_set_style_border_width(obj, 0, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

}  // namespace

lv_obj_t* makeMenuThermometer(lv_obj_t* parent) {
  lv_obj_t* icon = lv_obj_create(parent);
  lv_obj_set_size(icon, 16, 26);
  lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, 0);
  plain(icon);
  lv_obj_t* stem = lv_obj_create(icon);
  lv_obj_set_size(stem, 6, 16);
  lv_obj_set_style_radius(stem, 3, 0);
  lv_obj_set_style_bg_color(stem, lv_color_hex(kMuted), 0);
  plain(stem);
  lv_obj_align(stem, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_t* bulb = lv_obj_create(icon);
  lv_obj_set_size(bulb, 12, 12);
  lv_obj_set_style_radius(bulb, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(bulb, lv_color_hex(kMuted), 0);
  plain(bulb);
  lv_obj_align(bulb, LV_ALIGN_BOTTOM_MID, 0, 0);
  return icon;
}

}  // namespace wattcycle::display
