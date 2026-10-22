#pragma once

#include <cstdint>

namespace wattcycle::panel {

/// Who currently owns the camera / mic pipeline.
enum class P4LiveClaim : uint8_t {
  None = 0,
  Web,
  Panel,
};

}  // namespace wattcycle::panel
