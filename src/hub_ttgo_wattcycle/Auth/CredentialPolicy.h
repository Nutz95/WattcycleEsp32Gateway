#pragma once

#include <cstddef>

namespace wattcycle::auth {

/// Validates username/password before hashing or NVS write (overflow / injection hardening).
class CredentialPolicy {
 public:
  static constexpr size_t kMaxJsonBodyBytes = 512;

  static bool validateUsername(const char* username, char* error, size_t errorCapacity);
  static bool validatePassword(const char* password, char* error, size_t errorCapacity);
  static bool validateAuthJsonBody(const char* body, size_t bodyLength, char* error,
                                   size_t errorCapacity);
};

}  // namespace wattcycle::auth
