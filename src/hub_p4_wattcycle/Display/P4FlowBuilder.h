#pragma once

#include "Display/P4OverviewFlow.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::display {

/// Build the overview diagram from the store. Does not draw and does not touch ESP-NOW.
P4OverviewFlow buildP4Overview(const telemetry::ITelemetryStore& store);

}  // namespace wattcycle::display
