#pragma once

#include "Display/P4PanelBoard.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::display {

/// Paint one board from the telemetry store. Leaves MAC text and the Wi-Fi flag alone.
void fillP4LiveBoard(P4PanelBoard& board, const telemetry::ITelemetryStore& store);

}  // namespace wattcycle::display
