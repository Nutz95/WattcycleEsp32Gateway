#pragma once

#include "Display/P4PlugsFields.h"
#include "Display/P4SmokeFields.h"
#include "Shelly/IShellyPlugHub.h"
#include "Shelly/IShellySmokeHub.h"

namespace wattcycle::display {

/// Copy authorized Shelly readings into the panel document.
void fillPlugsFields(P4PlugsFields& fields, const shelly::IShellyPlugHub& hub, uint32_t nowMs);
/// Copy authorized smoke readings into the panel document.
void fillSmokeFields(P4SmokeFields& fields, const shelly::IShellySmokeHub& hub, uint32_t nowMs);

}  // namespace wattcycle::display
