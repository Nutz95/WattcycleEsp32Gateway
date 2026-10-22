#pragma once

namespace wattcycle::display {

/// Pack page. Later code copies the BMS snapshot into these fields.
struct P4WattcycleFields {
  bool linkUp = false;
  bool secured = false;
  bool valid = false;
  int soc = 0;
  int voltsMv = 0;
  int ampsMa = 0;
  int watts = 0;
  int remainAhTimes10 = 0;
  int totalAhTimes10 = 0;
  int cycles = 0;
  int soh = 0;
};

}  // namespace wattcycle::display
