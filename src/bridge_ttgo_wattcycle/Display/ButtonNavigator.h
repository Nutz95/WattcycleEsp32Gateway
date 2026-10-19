#pragma once

#include "Display/IButtonSource.h"

namespace wattcycle::display {

/// Debounced TTGO front-button edges (GPIO0 bottom, GPIO35 top).
class ButtonNavigator : public IButtonSource {
 public:
  void begin() override;
  ButtonEvent poll(uint32_t nowMs) override;

 private:
  uint32_t lastEdgeMs_ = 0;
  bool prevWasPressed_ = false;
  bool nextWasPressed_ = false;
};

}  // namespace wattcycle::display
