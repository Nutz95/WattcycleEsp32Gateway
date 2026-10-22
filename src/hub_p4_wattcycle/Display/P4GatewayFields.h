#pragma once

namespace wattcycle::display {

/// Link lamps for the gateway page and the top status band.
struct P4GatewayFields {
  bool wifiUp = false;
  bool bmsUp = false;
  bool ecoUp = false;
  bool solarUp = false;
};

}  // namespace wattcycle::display
