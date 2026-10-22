#pragma once

#include "Display/P4OverviewFlow.h"

namespace wattcycle::display {

/// Where live solar watts go. Idle when the meter is not producing.
struct P4SolarRoute {
  bool toPack = false;
  bool toEco = false;
};

/// Pack charging takes the solar. Otherwise a live meter bypasses up to the EcoFlow.
inline P4SolarRoute solarRoute(const P4OverviewFlow& flow) {
  P4SolarRoute route;
  const bool packCharging = flow.packEdge.active && !flow.packEdge.reverse;
  route.toPack = flow.solarActive && packCharging;
  route.toEco = flow.solarActive && !packCharging;
  return route;
}

}  // namespace wattcycle::display
