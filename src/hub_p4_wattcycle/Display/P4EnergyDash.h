#pragma once

#include "lvgl.h"

namespace wattcycle::display {

/// One bead travelling along an energy segment. Coordinates are parent space.
struct P4EnergyDash {
  lv_obj_t* bead = nullptr;
  lv_coord_t fromX = 0;
  lv_coord_t fromY = 0;
  lv_coord_t toX = 0;
  lv_coord_t toY = 0;
  int along = 10;
  bool running = false;
};

void mountEnergyDash(P4EnergyDash& dash, lv_obj_t* parent);
void runEnergyDash(P4EnergyDash& dash, int fromX, int fromY, int toX, int toY, bool active, uint32_t delayMs);

}  // namespace wattcycle::display
