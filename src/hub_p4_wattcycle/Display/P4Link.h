#pragma once

#include <cstdint>

namespace wattcycle::display {

/// Radio link painted on a flow node. Up is the live path, down is stale.
enum class P4Link : uint8_t { Down = 0, Up };

}  // namespace wattcycle::display
