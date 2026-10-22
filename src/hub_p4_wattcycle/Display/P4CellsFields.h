#pragma once

namespace wattcycle::display {

constexpr int kP4CellSlots = 8;

/// Cell voltages in millivolts. `count` is how many slots are live.
struct P4CellsFields {
  int count = 0;
  int voltsMv[kP4CellSlots] = {};
  bool balancing[kP4CellSlots] = {};
};

}  // namespace wattcycle::display
