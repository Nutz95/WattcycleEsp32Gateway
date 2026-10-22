#pragma once
#ifndef P4_PANEL_BOARD_H
#define P4_PANEL_BOARD_H

#include "Display/P4AlertsFields.h"
#include "Display/P4CellsFields.h"
#include "Display/P4EcoFields.h"
#include "Display/P4EspFields.h"
#include "Display/P4GatewayFields.h"
#include "Display/P4OverviewFlow.h"
#include "Display/P4SolarFields.h"
#include "Display/P4TempsFields.h"
#include "Display/P4WattcycleFields.h"

namespace wattcycle::display {

/// The only document the panel paints. Fill it from the demo or, later, from the radios.
struct P4PanelBoard {
  P4OverviewFlow overview;
  P4WattcycleFields wattcycle;
  P4SolarFields solar;
  P4EcoFields eco;
  P4CellsFields cells;
  P4TempsFields temps;
  P4AlertsFields alerts;
  P4GatewayFields gateway;
  P4EspFields esp;
};

}  // namespace wattcycle::display

#endif
