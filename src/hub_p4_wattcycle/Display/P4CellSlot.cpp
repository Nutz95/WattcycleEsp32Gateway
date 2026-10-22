#include "Display/P4CellWidgets.h"

#include "Display/P4OverviewStyle.h"

namespace wattcycle::display {
namespace {

void showObject(lv_obj_t* obj, bool visible) {
  if (obj == nullptr) {
    return;
  }
  if (visible) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  }
}

}  // namespace

void makeCellSlot(P4CellWidgets& cells, int index, int y) {
  lv_obj_t* row = lv_obj_create(cells.root);
  lv_obj_set_pos(row, 16, y);
  lv_obj_set_size(row, 448, 52);
  lv_obj_set_style_bg_color(row, lv_color_hex(kCard), 0);
  lv_obj_set_style_radius(row, 12, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* track = lv_obj_create(row);
  lv_obj_set_pos(track, 72, 16);
  lv_obj_set_size(track, 220, 18);
  lv_obj_set_style_bg_color(track, lv_color_hex(kIdleEdge), 0);
  lv_obj_set_style_radius(track, 8, 0);
  lv_obj_set_style_border_width(track, 0, 0);
  lv_obj_set_style_pad_all(track, 0, 0);
  lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* fill = lv_obj_create(track);
  lv_obj_set_pos(fill, 0, 0);
  lv_obj_set_size(fill, 40, 18);
  lv_obj_set_style_bg_color(fill, lv_color_hex(kAccent), 0);
  lv_obj_set_style_radius(fill, 8, 0);
  lv_obj_set_style_border_width(fill, 0, 0);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* name = lv_label_create(row);
  lv_obj_set_style_text_font(name, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(name, lv_color_hex(kMuted), 0);
  lv_obj_set_pos(name, 12, 16);
  lv_label_set_text_fmt(name, "C%d", index + 1);
  lv_obj_t* value = lv_label_create(row);
  lv_label_set_text(value, "--");
  lv_obj_set_style_text_font(value, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(value, lv_color_hex(kInk), 0);
  lv_obj_set_pos(value, 300, 16);
  lv_obj_t* mark = lv_obj_create(row);
  lv_obj_set_pos(mark, 414, 16);
  lv_obj_set_size(mark, 18, 18);
  lv_obj_set_style_radius(mark, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(mark, 0, 0);
  lv_obj_set_style_bg_color(mark, lv_color_hex(kIdleEdge), 0);
  lv_obj_add_flag(mark, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(mark, LV_OBJ_FLAG_SCROLLABLE);
  cells.rows[index] = row;
  cells.fills[index] = fill;
  cells.labels[index] = value;
  cells.marks[index] = mark;
}

void paintCellMark(lv_obj_t* mark, bool balancing, bool isMin, bool isMax) {
  if (mark == nullptr) {
    return;
  }
  uint32_t color = kIdleEdge;
  bool show = false;
  if (balancing) {
    color = 0xF0B429;
    show = true;
  } else if (isMin) {
    color = kDanger;
    show = true;
  } else if (isMax) {
    color = kAccent;
    show = true;
  }
  if (!show) {
    lv_obj_add_flag(mark, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_set_style_bg_color(mark, lv_color_hex(color), 0);
  lv_obj_clear_flag(mark, LV_OBJ_FLAG_HIDDEN);
}

void setCellSlot(P4CellWidgets& cells, int index, int voltsMv, bool balancing, bool isMin,
                 bool isMax, bool visible) {
  showObject(cells.rows[index], visible);
  if (!visible || cells.fills[index] == nullptr) {
    return;
  }
  paintCellMark(cells.marks[index], balancing, isMin, isMax);
  int span = voltsMv - 3000;
  if (span < 0) {
    span = 0;
  }
  if (span > 600) {
    span = 600;
  }
  const int width = 24 + (span * 196) / 600;
  lv_obj_set_width(cells.fills[index], width);
  lv_obj_set_style_bg_color(cells.fills[index], lv_color_hex(kAccent), 0);
  lv_label_set_text_fmt(cells.labels[index], "%d.%03d V", voltsMv / 1000, voltsMv % 1000);
}

}  // namespace wattcycle::display
