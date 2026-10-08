#pragma once

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

class PasswordHasher {
 public:
  static bool hashPassword(const char* password, const uint8_t salt[kSaltBytes],
                           uint8_t outHash[kHashBytes]);
  static bool verifyPassword(const char* password, const uint8_t salt[kSaltBytes],
                             const uint8_t expectedHash[kHashBytes]);
};

}  // namespace wattcycle::auth
