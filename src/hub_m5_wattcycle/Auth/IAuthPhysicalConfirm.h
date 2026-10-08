#pragma once

#include <cstdint>

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

/// Device UI surface: physical confirm / cancel / long-press reset.
class IAuthPhysicalConfirm {
 public:
  virtual ~IAuthPhysicalConfirm() = default;

  virtual AuthPrompt prompt() const = 0;
  virtual void onCancelHeld(bool held, uint32_t nowMs) = 0;
  virtual void onConfirmPressed() = 0;
  virtual void onCancelPressed() = 0;
};

}  // namespace wattcycle::auth
