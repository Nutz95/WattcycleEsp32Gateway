#include "Auth/CredentialPolicy.h"

#include "Auth/AuthTypes.h"
#include "Util/SafeCopy.h"

#include <cstring>

namespace wattcycle::auth {
namespace {

void copyError(char* error, size_t capacity, const char* message) {
  wattcycle::util::copyCString(error, capacity, message);
}

bool isAllowedUsernameChar(unsigned char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
         ch == '.' || ch == '_' || ch == '-';
}

bool isAllowedPasswordChar(unsigned char ch) {
  // Printable ASCII only — rejects NUL, controls, and high-bit bytes that complicate
  // hashing / JSON / display without adding real entropy for this gateway.
  return ch >= 0x20 && ch <= 0x7E;
}

}  // namespace

bool CredentialPolicy::validateUsername(const char* username, char* error, size_t errorCapacity) {
  if (username == nullptr) {
    copyError(error, errorCapacity, "invalid_input");
    return false;
  }
  const size_t len = std::strlen(username);
  if (len == 0 || len > kMaxUsernameLen) {
    copyError(error, errorCapacity, "invalid_username");
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    if (!isAllowedUsernameChar(static_cast<unsigned char>(username[i]))) {
      copyError(error, errorCapacity, "invalid_username");
      return false;
    }
  }
  copyError(error, errorCapacity, "");
  return true;
}

bool CredentialPolicy::validatePassword(const char* password, char* error, size_t errorCapacity) {
  if (password == nullptr) {
    copyError(error, errorCapacity, "invalid_input");
    return false;
  }
  const size_t len = std::strlen(password);
  if (len < kMinPasswordLen) {
    copyError(error, errorCapacity, "password_too_short");
    return false;
  }
  if (len > kMaxPasswordLen) {
    copyError(error, errorCapacity, "password_too_long");
    return false;
  }
  for (size_t i = 0; i < len; ++i) {
    if (!isAllowedPasswordChar(static_cast<unsigned char>(password[i]))) {
      copyError(error, errorCapacity, "invalid_password_chars");
      return false;
    }
  }
  copyError(error, errorCapacity, "");
  return true;
}

bool CredentialPolicy::validateAuthJsonBody(const char* body, size_t bodyLength, char* error,
                                            size_t errorCapacity) {
  if (body == nullptr) {
    copyError(error, errorCapacity, "invalid_json");
    return false;
  }
  if (bodyLength == 0 || bodyLength > kMaxJsonBodyBytes) {
    copyError(error, errorCapacity, "payload_too_large");
    return false;
  }
  // Reject embedded NUL early (truncated C-strings / parser tricks).
  for (size_t i = 0; i < bodyLength; ++i) {
    if (body[i] == '\0') {
      copyError(error, errorCapacity, "invalid_json");
      return false;
    }
  }
  copyError(error, errorCapacity, "");
  return true;
}

}  // namespace wattcycle::auth
