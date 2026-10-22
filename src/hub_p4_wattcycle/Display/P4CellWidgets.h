#pragma once

#include "Display/P4CellsFields.h"

#include "lvgl.h"

namespace wattcycle::display {

/// Cell page: one summary line and a bar per slot.
struct P4CellWidgets {
  lv_obj_t* root = nullptr;
  lv_obj_t* summary = nullptr;
  lv_obj_t* rows[kP4CellSlots] = {};
  lv_obj_t* fills[kP4CellSlots] = {};
  lv_obj_t* labels[kP4CellSlots] = {};
};

void makeCellSlot(P4CellWidgets& cells, int index, int y);
void setCellSlot(P4CellWidgets& cells, int index, int voltsMv, bool balancing, bool visible);

}  // namespace wattcycle::display
