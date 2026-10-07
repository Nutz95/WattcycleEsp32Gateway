#pragma once

#include <cstddef>

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

class IAuthService {
 public:
  virtual ~IAuthService() = default;

  virtual void begin() = 0;
  virtual AuthStatus status(const char* sessionToken) const = 0;
  virtual AuthPrompt prompt() const = 0;

  virtual bool beginSetup(const char* username, const char* password, char* error,
                          size_t errorCapacity) = 0;
  virtual bool confirmSetup() = 0;
  virtual void cancelPending() = 0;

  virtual bool beginResetRequest() = 0;
  virtual bool confirmReset() = 0;

  virtual bool login(const char* username, const char* password, char* sessionTokenOut,
                     size_t sessionTokenCapacity, char* error, size_t errorCapacity) = 0;
  virtual void logout(const char* sessionToken) = 0;
  virtual bool isAuthenticated(const char* sessionToken) const = 0;

  /// Called from the app loop for long-press reset detection (GPIO0).
  virtual void onCancelHeld(bool held, uint32_t nowMs) = 0;
  virtual void onConfirmPressed() = 0;
  virtual void onCancelPressed() = 0;
};

}  // namespace wattcycle::auth
