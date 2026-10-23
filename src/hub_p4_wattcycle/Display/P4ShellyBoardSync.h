#pragma once

#include "Display/P4PlugsFields.h"
#include "Shelly/IShellyPlugHub.h"

#include <cstdint>

namespace wattcycle::display {

/// Copy authorized Shelly readings into the panel document.
void fillPlugsFields(P4PlugsFields& fields, const shelly::IShellyPlugHub& hub, uint32_t nowMs);

}  // namespace wattcycle::display
