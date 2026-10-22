#pragma once

#include "lvgl.h"

#include <cstdint>

namespace wattcycle::display {

/// Transparent content root under the status band.
lv_obj_t* makeDetailRoot(lv_obj_t* parent);
/// Named row card. Returns the value label to fill later.
lv_obj_t* makeLabeledRow(lv_obj_t* parent, int y, const char* name);
/// Paint one value label.
void setDetailValue(lv_obj_t* label, const char* text, uint32_t color);

}  // namespace wattcycle::display
