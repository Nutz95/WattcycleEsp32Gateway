#pragma once

#include "Display/P4PageModel.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::display {

/// English title for one hub page.
const char* hubPageTitle(HubPage page);
/// Fill the three lines for a detail page. Overview stays on `P4OverviewFlow`.
P4PageModel buildP4Page(HubPage page, const telemetry::ITelemetryStore& store);

}  // namespace wattcycle::display
