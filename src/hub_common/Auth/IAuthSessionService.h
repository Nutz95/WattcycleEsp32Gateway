#pragma once

#include <cstddef>

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

/// Web/session surface: credentials + login cookies (no GPIO / no physical confirm).
class IAuthSessionService {
 public:
  virtual ~IAuthSessionService() = default;

  /// Load stored credentials and prepare session state.
  virtual void begin() = 0;
  /// Snapshot of configured / pending / authenticated for this cookie.
  virtual AuthStatus status(const char* sessionToken) const = 0;

  /// Start first-boot credential setup; device must still confirm.
  virtual bool beginSetup(const char* username, const char* password, char* error,
                          size_t errorCapacity) = 0;

  /// Verify credentials and issue a session token hex string.
  virtual bool login(const char* username, const char* password, char* sessionTokenOut,
                     size_t sessionTokenCapacity, char* error, size_t errorCapacity) = 0;
  /// Drop one session cookie.
  virtual void logout(const char* sessionToken) = 0;
  /// True when the cookie is a live authenticated session.
  virtual bool isAuthenticated(const char* sessionToken) const = 0;
};

}  // namespace wattcycle::auth
