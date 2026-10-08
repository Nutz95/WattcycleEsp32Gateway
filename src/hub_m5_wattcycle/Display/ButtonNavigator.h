#pragma once

#include <cstdint>

namespace wattcycle::display {

enum class ButtonAction : uint8_t { None, Previous, Next, Confirm, Cancel };

struct ButtonEvent {
  bool anyPress = false;
  ButtonAction action = ButtonAction::None;
};

/// M5Stack Basic A/B/C via M5Unified (A=prev, C=next, B=confirm).
class ButtonNavigator {
 public:
  void begin();
  ButtonEvent poll(uint32_t nowMs);

 private:
  uint32_t lastEdgeMs_ = 0;
};

}  // namespace wattcycle::display
