#pragma once

#include "lvgl.h"

namespace wattcycle::display {

constexpr int kDetailRows = 10;

/// One text page: a root plus up to eight value labels.
struct P4TextPage {
  lv_obj_t* root = nullptr;
  lv_obj_t* values[kDetailRows] = {};
};

}  // namespace wattcycle::display
