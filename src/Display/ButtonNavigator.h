#pragma once

#include <cstdint>

namespace wattcycle::display {

enum class ButtonAction : uint8_t {
  None = 0,
  Previous,
  Next,
};

struct ButtonEvent {
  ButtonAction action = ButtonAction::None;
  bool anyPress = false;
};

/// Debounced TTGO front-button edges (GPIO0 prev, GPIO35 next).
class ButtonNavigator {
 public:
  void begin();
  ButtonEvent poll(uint32_t nowMs);

 private:
  uint32_t lastEdgeMs_ = 0;
  bool prevWasPressed_ = false;
  bool nextWasPressed_ = false;
};

}  // namespace wattcycle::display
