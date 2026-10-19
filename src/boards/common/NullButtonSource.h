#pragma once

#include "boards/common/IButtonSource.h"

namespace boards::input {

/// No physical buttons (headless boards).
class NullButtonSource : public IButtonSource {
 public:
  /// No-op GPIO setup.
  void begin() override {}
  /// Always empty event.
  ButtonEvent poll(uint32_t) override { return {}; }
};

}  // namespace boards::input
