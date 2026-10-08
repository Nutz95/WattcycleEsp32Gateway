#pragma once

#include <cstdint>

namespace xt369p::display {

enum class ButtonAction : uint8_t {
  None = 0,
  Previous,  // bottom / cancel edge
  Next,      // top / confirm edge
};

struct ButtonEvent {
  ButtonAction action = ButtonAction::None;
  bool anyPress = false;
  bool cancelHeld = false;
};

/// Debounced TTGO front-button edges (GPIO0 bottom, GPIO35 top).
class ButtonNavigator {
 public:
  void begin();
  ButtonEvent poll(uint32_t nowMs);

 private:
  uint32_t lastEdgeMs_ = 0;
  bool prevWasPressed_ = false;
  bool nextWasPressed_ = false;
};

}  // namespace xt369p::display
