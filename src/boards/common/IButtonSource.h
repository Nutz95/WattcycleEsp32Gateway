#pragma once

#include "boards/common/ButtonEvent.h"

#include <cstdint>

namespace boards::input {

/// Debounced board button input (TTGO dual-button or T-Dongle single-button).
class IButtonSource {
 public:
  virtual ~IButtonSource() = default;
  /// Configure GPIOs.
  virtual void begin() = 0;
  /// Poll edges / long-press; nowMs from millis().
  virtual ButtonEvent poll(uint32_t nowMs) = 0;
};

}  // namespace boards::input
