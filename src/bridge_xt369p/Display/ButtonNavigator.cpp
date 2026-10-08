#include "Display/ButtonNavigator.h"

#include "Config/TimingConstants.h"
#include "Display/DisplayPins.h"

#ifndef UNIT_TEST

#include <Arduino.h>

namespace xt369p::display {

void ButtonNavigator::begin() {
  pinMode(kButtonCancelGpio, INPUT_PULLUP);
  pinMode(kButtonConfirmGpio, INPUT);
}

ButtonEvent ButtonNavigator::poll(uint32_t nowMs) {
  ButtonEvent event;
  const bool cancelPressed = digitalRead(kButtonCancelGpio) == LOW;
  const bool confirmPressed = digitalRead(kButtonConfirmGpio) == LOW;
  event.cancelHeld = cancelPressed;

  if ((nowMs - lastEdgeMs_) < config::TimingConstants::kButtonDebounceMs) {
    prevWasPressed_ = cancelPressed;
    nextWasPressed_ = confirmPressed;
    return event;
  }

  if (cancelPressed && !prevWasPressed_) {
    event.action = ButtonAction::Previous;
    event.anyPress = true;
    lastEdgeMs_ = nowMs;
  } else if (confirmPressed && !nextWasPressed_) {
    event.action = ButtonAction::Next;
    event.anyPress = true;
    lastEdgeMs_ = nowMs;
  }

  prevWasPressed_ = cancelPressed;
  nextWasPressed_ = confirmPressed;
  return event;
}

}  // namespace xt369p::display

#else

namespace xt369p::display {

void ButtonNavigator::begin() {}

ButtonEvent ButtonNavigator::poll(uint32_t) {
  return {};
}

}  // namespace xt369p::display

#endif
