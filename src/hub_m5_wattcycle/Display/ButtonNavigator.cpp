#include "Display/ButtonNavigator.h"

#include "Config/TimingConstants.h"

#include <M5Unified.h>

namespace wattcycle::display {

void ButtonNavigator::begin() {
  lastEdgeMs_ = 0;
}

ButtonEvent ButtonNavigator::poll(uint32_t nowMs) {
  ButtonEvent event{};
  M5.update();
  if ((nowMs - lastEdgeMs_) < config::TimingConstants::kButtonDebounceMs) {
    return event;
  }
  if (M5.BtnA.wasPressed()) {
    lastEdgeMs_ = nowMs;
    event.anyPress = true;
    event.action = ButtonAction::Previous;
  } else if (M5.BtnC.wasPressed()) {
    lastEdgeMs_ = nowMs;
    event.anyPress = true;
    event.action = ButtonAction::Next;
  } else if (M5.BtnB.wasPressed()) {
    lastEdgeMs_ = nowMs;
    event.anyPress = true;
    event.action = ButtonAction::Confirm;
  }
  return event;
}

}  // namespace wattcycle::display
