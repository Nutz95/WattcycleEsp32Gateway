#include "Display/ButtonNavigator.h"

#include "Config/TimingConstants.h"
#include "Display/DisplayPins.h"

#ifndef UNIT_TEST

#include <Arduino.h>

namespace wattcycle::display {

void ButtonNavigator::begin() {
  pinMode(kButtonPrevGpio, INPUT_PULLUP);
  pinMode(kButtonNextGpio, INPUT);
}

ButtonEvent ButtonNavigator::poll(uint32_t nowMs) {
  ButtonEvent event;
  const bool prevPressed = digitalRead(kButtonPrevGpio) == LOW;
  const bool nextPressed = digitalRead(kButtonNextGpio) == LOW;

  if ((nowMs - lastEdgeMs_) < config::TimingConstants::kButtonDebounceMs) {
    prevWasPressed_ = prevPressed;
    nextWasPressed_ = nextPressed;
    return event;
  }

  if (prevPressed && !prevWasPressed_) {
    event.action = ButtonAction::Previous;
    event.anyPress = true;
    lastEdgeMs_ = nowMs;
  } else if (nextPressed && !nextWasPressed_) {
    event.action = ButtonAction::Next;
    event.anyPress = true;
    lastEdgeMs_ = nowMs;
  }

  prevWasPressed_ = prevPressed;
  nextWasPressed_ = nextPressed;
  return event;
}

}  // namespace wattcycle::display

#else

namespace wattcycle::display {

void ButtonNavigator::begin() {}

ButtonEvent ButtonNavigator::poll(uint32_t) {
  return {};
}

}  // namespace wattcycle::display

#endif
