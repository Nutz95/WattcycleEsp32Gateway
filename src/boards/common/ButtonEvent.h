#pragma once

#include "boards/common/ButtonAction.h"

namespace boards::input {

/// One poll result from an IButtonSource.
struct ButtonEvent {
  ButtonAction action = ButtonAction::None;
  bool anyPress = false;
  bool cancelHeld = false;
};

}  // namespace boards::input
