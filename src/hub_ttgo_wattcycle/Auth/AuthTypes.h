#pragma once

#include <cstdint>
#include <cstring>

namespace wattcycle::auth {

constexpr size_t kMaxUsernameLen = 32;
constexpr size_t kSaltBytes = 16;
constexpr size_t kHashBytes = 32;
constexpr size_t kSessionTokenBytes = 16;
constexpr size_t kSessionTokenHexLen = kSessionTokenBytes * 2;
constexpr uint32_t kSessionTtlMs = 24u * 60u * 60u * 1000u;
constexpr uint32_t kMinPasswordLen = 8;
constexpr size_t kMaxPasswordLen = 64;
constexpr uint32_t kMaxLoginFailures = 5;
constexpr uint32_t kLoginLockoutMs = 30000;
constexpr uint32_t kResetHoldMs = 3000;

enum class AuthPromptKind : uint8_t {
  None = 0,
  ConfirmSetup,
  ConfirmReset,
};

struct AuthStatus {
  bool configured = false;
  bool pendingSetup = false;
  bool pendingReset = false;
  bool authenticated = false;
};

struct AuthPrompt {
  AuthPromptKind kind = AuthPromptKind::None;
  char username[kMaxUsernameLen + 1] = {};
};

struct StoredCredentials {
  char username[kMaxUsernameLen + 1] = {};
  uint8_t salt[kSaltBytes] = {};
  uint8_t hash[kHashBytes] = {};
};

struct PendingSetup {
  bool active = false;
  char username[kMaxUsernameLen + 1] = {};
  char password[kMaxPasswordLen + 1] = {};
};

}  // namespace wattcycle::auth
