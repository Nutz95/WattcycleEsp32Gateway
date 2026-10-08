#pragma once

#include <cstddef>

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

/// Web/session surface: credentials + login cookies (no GPIO / no physical confirm).
class IAuthSessionService {
 public:
  virtual ~IAuthSessionService() = default;

  virtual void begin() = 0;
  virtual AuthStatus status(const char* sessionToken) const = 0;

  virtual bool beginSetup(const char* username, const char* password, char* error,
                          size_t errorCapacity) = 0;

  virtual bool login(const char* username, const char* password, char* sessionTokenOut,
                     size_t sessionTokenCapacity, char* error, size_t errorCapacity) = 0;
  virtual void logout(const char* sessionToken) = 0;
  virtual bool isAuthenticated(const char* sessionToken) const = 0;
};

}  // namespace wattcycle::auth
