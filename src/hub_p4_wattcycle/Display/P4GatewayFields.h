#pragma once

#include "Display/P4MacText.h"

namespace wattcycle::display {

/// Link lamps and ESP-NOW peer addresses for the gateway page.
/// `hubMac` is this panel's STA address; peer slots are the bridges that should TX here.
struct P4GatewayFields {
  bool wifiUp = false;
  bool bmsUp = false;
  bool ecoUp = false;
  bool solarUp = false;
  char hubMac[kP4MacText] = {};
  char bmsMac[kP4MacText] = {};
  char ecoMac[kP4MacText] = {};
  char solarMac[kP4MacText] = {};
};

}  // namespace wattcycle::display
