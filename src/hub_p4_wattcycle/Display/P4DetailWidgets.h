#pragma once

#include "Display/P4CellWidgets.h"
#include "Display/P4TextPage.h"

namespace wattcycle::display {

/// Every detail page the menu can open. Overview stays on `P4OverviewView`.
struct P4DetailWidgets {
  P4TextPage wattcycle;
  P4TextPage solar;
  P4TextPage eco;
  P4CellWidgets cells;
  P4TextPage temps;
  P4TextPage alerts;
  P4TextPage gateway;
  P4TextPage esp;
  P4TextPage link;
};

}  // namespace wattcycle::display
