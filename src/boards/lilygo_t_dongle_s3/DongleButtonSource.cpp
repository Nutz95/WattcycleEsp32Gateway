#include "boards/lilygo_t_dongle_s3/DongleButtonSource.h"

#include "boards/lilygo_t_dongle_s3/BoardPins.h"

#include <Arduino.h>

namespace boards::lilygo_t_dongle_s3 {

void DongleButtonSource::begin() {
  pinMode(kButtonGpio, INPUT_PULLUP);
}

boards::input::ButtonEvent DongleButtonSource::poll(uint32_t nowMs) {
  using boards::input::ButtonAction;
  using boards::input::ButtonEvent;

  ButtonEvent event;
  const bool pressed = digitalRead(kButtonGpio) == LOW;

  if (pressed && !wasPressed_) {
    pressStartedMs_ = nowMs;
    longFired_ = false;
  }

  if (pressed && !longFired_ && pressStartedMs_ != 0 &&
      (nowMs - pressStartedMs_) >= kLongPressMs) {
    event.action = ButtonAction::Sleep;
    event.anyPress = true;
    longFired_ = true;
    lastEdgeMs_ = nowMs;
  }

  if (!pressed && wasPressed_ && !longFired_) {
    if ((nowMs - lastEdgeMs_) >= 180u || lastEdgeMs_ == 0) {
      event.action = ButtonAction::Next;
      event.anyPress = true;
      lastEdgeMs_ = nowMs;
    }
  }

  wasPressed_ = pressed;
  return event;
}

}  // namespace boards::lilygo_t_dongle_s3
