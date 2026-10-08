#include "Display/ButtonNavigator.h"

#include "Config/TimingConstants.h"

#ifndef UNIT_TEST
#include <M5Unified.h>
#endif

namespace wattcycle::display {

void ButtonNavigator::begin() {
  lastEdgeMs_ = 0;
}

ButtonEvent ButtonNavigator::poll(uint32_t nowMs) {
  ButtonEvent event{};
#ifndef UNIT_TEST
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
#else
  (void)nowMs;
#endif
  return event;
}

}  // namespace wattcycle::display
