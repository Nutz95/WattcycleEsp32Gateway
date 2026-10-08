#pragma once

#include "Auth/AuthTypes.h"

namespace wattcycle::auth {

struct AuthSession {
  bool used = false;
  char tokenHex[kSessionTokenHexLen + 1] = {};
  uint32_t expiresAtMs = 0;
};

/// RAM-only session tokens (lost on reboot — intentional).
class SessionStore {
 public:
  static constexpr size_t kMaxSessions = 4;

  void clear();
  bool create(char* tokenOut, size_t tokenCapacity, uint32_t nowMs);
  bool valid(const char* token, uint32_t nowMs) const;
  void revoke(const char* token);

 private:
  AuthSession sessions_[kMaxSessions] = {};
};

}  // namespace wattcycle::auth
