#pragma once

#include <cstdint>

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

/// Device UI surface: physical confirm / cancel / long-press reset.
class IAuthPhysicalConfirm {
 public:
  virtual ~IAuthPhysicalConfirm() = default;

  /// Current on-device auth prompt (setup, reset, or idle).
  virtual AuthPrompt prompt() const = 0;
  /// Bottom-button hold for the forgot-password long-press path.
  virtual void onCancelHeld(bool held, uint32_t nowMs) = 0;
  /// Top-button confirm for pending setup or reset.
  virtual void onConfirmPressed() = 0;
  /// Cancel the pending setup or reset prompt.
  virtual void onCancelPressed() = 0;
  /// Start a login reset; the device must still confirm before credentials are wiped.
  virtual bool beginResetRequest() = 0;
};

}  // namespace wattcycle::auth
