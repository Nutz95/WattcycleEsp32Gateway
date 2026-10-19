#pragma once

#include <cstdint>

namespace boards::input {

/// Debounced button intent (short press or long-press sleep).
enum class ButtonAction : uint8_t {
  None = 0,
  Previous,
  Next,
  Sleep,  // long-press on single-button boards
};

}  // namespace boards::input
