#include "Auth/PasswordHasher.h"

#include "Auth/Sha256.h"

#include <cstring>

namespace wattcycle::auth {

bool PasswordHasher::hashPassword(const char* password, const uint8_t salt[kSaltBytes],
                                  uint8_t outHash[kHashBytes]) {
  if (password == nullptr || salt == nullptr || outHash == nullptr) {
    return false;
  }
  const size_t passwordLen = std::strlen(password);
  uint8_t material[kSaltBytes + kMaxPasswordLen];
  if (passwordLen > kMaxPasswordLen) {
    return false;
  }
  std::memcpy(material, salt, kSaltBytes);
  std::memcpy(material + kSaltBytes, password, passwordLen);
  Sha256::hash(material, kSaltBytes + passwordLen, outHash);
  return true;
}

bool PasswordHasher::verifyPassword(const char* password, const uint8_t salt[kSaltBytes],
                                    const uint8_t expectedHash[kHashBytes]) {
  uint8_t actual[kHashBytes] = {};
  if (!hashPassword(password, salt, actual)) {
    return false;
  }
  uint8_t diff = 0;
  for (size_t i = 0; i < kHashBytes; ++i) {
    diff |= static_cast<uint8_t>(actual[i] ^ expectedHash[i]);
  }
  return diff == 0;
}

}  // namespace wattcycle::auth
