#pragma once

namespace wattcycle::display {

/// Solar page.
struct P4SolarFields {
  bool linkUp = false;
  int watts = 0;
  int voltsMv = 0;
};

}  // namespace wattcycle::display
