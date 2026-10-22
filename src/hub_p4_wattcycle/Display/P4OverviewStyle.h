#pragma once

#include "Display/P4OverviewFlow.h"

#include "lvgl.h"

namespace wattcycle::display {

constexpr uint32_t kBg = 0x10141C;
constexpr uint32_t kCard = 0x1C2433;
constexpr uint32_t kInk = 0xE8EEFC;
constexpr uint32_t kMuted = 0x93A4C7;
constexpr uint32_t kAccent = 0x3ECF8E;
constexpr uint32_t kDanger = 0xFF6B6B;
constexpr uint32_t kSelf = 0xE6C07B;
constexpr uint32_t kBalanceNeg = 0xFB7185;
constexpr uint32_t kIdleEdge = 0x3A4458;

constexpr int kStatusH = 52;
constexpr int kMenuY = 720;
constexpr int kMenuH = 80;
constexpr int kColX = 28;
constexpr int kColW = 196;
constexpr int kSpineX = kColX + (kColW / 2);
constexpr int kPortX = 348;
constexpr int kPortW = 116;
constexpr int kRailX = 12;
constexpr int kGridX = kSpineX - 80;
constexpr int kGridY = 62;
constexpr int kGridW = 160;
constexpr int kGridH = 76;
constexpr int kEcoY = 180;
constexpr int kEcoH = 228;
constexpr int kPortH = 72;
constexpr int kPortPitch = 78;
constexpr int kPackY = 448;
constexpr int kPackH = 108;
constexpr int kSolarY = 596;
constexpr int kSolarH = 88;
constexpr int kEcoMidY = kEcoY + (kEcoH / 2);
constexpr int kSolarMidY = kSolarY + (kSolarH / 2);
constexpr int kLineGap = 3;
constexpr int kLaneCount = 7;
constexpr int kLaneGrid = 0;
constexpr int kLaneAc = 1;
constexpr int kLaneDc = 2;
constexpr int kLaneUsb = 3;
constexpr int kLanePack = 4;
constexpr int kLaneSolar = 5;
constexpr int kLaneBypass = 6;
constexpr int kBeadsPerLane = 2;

inline lv_obj_t* makeCard(lv_obj_t* parent, int x, int y, int width, int height) {
  lv_obj_t* card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, width, height);
  lv_obj_set_style_bg_color(card, lv_color_hex(kCard), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(card, 2, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(kIdleEdge), 0);
  lv_obj_set_style_radius(card, 14, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_set_style_outline_width(card, 0, 0);
  lv_obj_set_style_shadow_width(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  return card;
}

inline lv_obj_t* makeValueLabel(lv_obj_t* parent) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, "");
  lv_obj_set_style_text_color(label, lv_color_hex(kMuted), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  return label;
}

inline void stampTitle(lv_obj_t* card, const char* text, int y, int width) {
  for (int offset = 0; offset <= 1; ++offset) {
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, text);
    lv_obj_set_size(title, width, lv_font_get_line_height(&lv_font_montserrat_20));
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(kInk), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(title, offset, y);
  }
}

inline void layoutCardText(lv_obj_t* card, const char* title, lv_obj_t** values, int count) {
  const lv_coord_t border = lv_obj_get_style_border_width(card, 0);
  const lv_coord_t innerW = lv_obj_get_style_width(card, 0) - (2 * border);
  const lv_coord_t innerH = lv_obj_get_style_height(card, 0) - (2 * border);
  const lv_coord_t titleH = lv_font_get_line_height(&lv_font_montserrat_20);
  const lv_coord_t valueH = lv_font_get_line_height(&lv_font_montserrat_14);
  const lv_coord_t gap = 4;
  lv_coord_t block = titleH;
  if (count > 0) {
    block += gap + (count * valueH) + ((count - 1) * gap);
  }
  const lv_coord_t top = (innerH - block) / 2;
  stampTitle(card, title, top, innerW);
  lv_coord_t y = top + titleH + gap;
  for (int index = 0; index < count; ++index) {
    lv_obj_t* value = values[index];
    lv_obj_set_size(value, innerW, valueH);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(value, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(value, 0, y);
    y += valueH + gap;
  }
}

inline lv_obj_t* makeEdge(lv_obj_t* parent, lv_point_t* points) {
  lv_obj_t* line = lv_line_create(parent);
  lv_obj_set_pos(line, 0, 0);
  lv_line_set_points(line, points, 2);
  lv_obj_set_style_line_width(line, 2, 0);
  lv_obj_set_style_line_rounded(line, true, 0);
  lv_obj_set_style_line_color(line, lv_color_hex(kIdleEdge), 0);
  lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
  return line;
}

inline void paintEnergy(lv_obj_t* line, bool active) {
  (void)active;
  lv_obj_set_style_line_color(line, lv_color_hex(kIdleEdge), 0);
  lv_obj_set_style_line_width(line, 2, 0);
}

inline void paintRadio(lv_obj_t* card, P4Link link) {
  const uint32_t color = link == P4Link::Up ? kAccent : kDanger;
  lv_obj_set_style_border_color(card, lv_color_hex(color), 0);
}

inline void paintFlowNode(lv_obj_t* card, bool active) {
  const uint32_t color = active ? kAccent : kIdleEdge;
  lv_obj_set_style_border_color(card, lv_color_hex(color), 0);
}

inline const char* linkName(P4Link link) {
  return link == P4Link::Up ? "live" : "no link";
}

}  // namespace wattcycle::display
