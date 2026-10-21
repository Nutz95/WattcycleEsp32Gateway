#pragma once

#include "Display/HubPage.h"

namespace wattcycle::display {

/// Three text lines a later LVGL view binds. No widgets live here.
struct P4PageModel {
  HubPage page = HubPage::Overview;
  char title[16] = {};
  char line1[48] = {};
  char line2[48] = {};
  char line3[48] = {};
};

}  // namespace wattcycle::display
