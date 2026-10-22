#pragma once

namespace wattcycle::display {

/// EcoFlow detail page. Battery watts keep the station sign.
struct P4EcoFields {
  bool linkUp = false;
  bool secured = false;
  bool valid = false;
  int soc = 0;
  int batteryW = 0;
  bool acOn = false;
  int acW = 0;
  bool dcOn = false;
  int dcW = 0;
  bool usbOn = false;
  int usbW = 0;
};

}  // namespace wattcycle::display
