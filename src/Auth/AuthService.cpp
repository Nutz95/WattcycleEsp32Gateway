#include "Auth/AuthService.h"

#include "Auth/CredentialPolicy.h"
#include "Auth/PasswordHasher.h"
#include "Util/RandomBytes.h"
#include "Util/SafeCopy.h"

#include <cstring>

#ifndef UNIT_TEST
#include <Arduino.h>
#else
static uint32_t millis() {
  return 0;
}
#endif

namespace wattcycle::auth {

AuthService::AuthService(ICredentialStore& credentialStore)
    : credentialStore_(credentialStore) {}

void AuthService::begin() {
  sessions_.clear();
  pendingSetup_ = {};
  pendingReset_ = false;
  cancelHoldStartedMs_ = 0;
  loginFailures_ = 0;
  loginLockUntilMs_ = 0;
}

uint32_t AuthService::nowMs() const {
  return millis();
}

bool AuthService::recordLoginFailure(uint32_t now, char* error, size_t errorCapacity) {
  loginFailures_ += 1;
  if (loginFailures_ >= kMaxLoginFailures) {
    loginLockUntilMs_ = now + kLoginLockoutMs;
    loginFailures_ = 0;
    util::copyCString(error, errorCapacity, "locked");
  } else {
    util::copyCString(error, errorCapacity, "invalid_credentials");
  }
  return false;
}

AuthStatus AuthService::status(const char* sessionToken) const {
  AuthStatus out;
  out.configured = credentialStore_.hasCredentials();
  out.pendingSetup = pendingSetup_.active;
  out.pendingReset = pendingReset_;
  out.authenticated = isAuthenticated(sessionToken);
  return out;
}

AuthPrompt AuthService::prompt() const {
  AuthPrompt out;
  if (pendingSetup_.active) {
    out.kind = AuthPromptKind::ConfirmSetup;
    util::copyCString(out.username, sizeof(out.username), pendingSetup_.username);
  } else if (pendingReset_) {
    out.kind = AuthPromptKind::ConfirmReset;
  }
  return out;
}

bool AuthService::beginSetup(const char* username, const char* password, char* error,
                             size_t errorCapacity) {
  if (credentialStore_.hasCredentials()) {
    util::copyCString(error, errorCapacity, "already_configured");
    return false;
  }
  if (pendingSetup_.active || pendingReset_) {
    util::copyCString(error, errorCapacity, "pending_confirmation");
    return false;
  }
  if (!CredentialPolicy::validateUsername(username, error, errorCapacity)) {
    return false;
  }
  if (!CredentialPolicy::validatePassword(password, error, errorCapacity)) {
    return false;
  }

  pendingSetup_ = {};
  pendingSetup_.active = true;
  util::copyCString(pendingSetup_.username, sizeof(pendingSetup_.username), username);
  util::copyCString(pendingSetup_.password, sizeof(pendingSetup_.password), password);
  util::copyCString(error, errorCapacity, "");
  return true;
}

bool AuthService::confirmSetup() {
  if (!pendingSetup_.active) {
    return false;
  }
  StoredCredentials credentials;
  std::memset(&credentials, 0, sizeof(credentials));
  util::copyCString(credentials.username, sizeof(credentials.username), pendingSetup_.username);
  util::fillRandom(credentials.salt, kSaltBytes);
  if (!PasswordHasher::hashPassword(pendingSetup_.password, credentials.salt, credentials.hash)) {
    return false;
  }
  if (!credentialStore_.save(credentials)) {
    return false;
  }
  std::memset(pendingSetup_.password, 0, sizeof(pendingSetup_.password));
  pendingSetup_ = {};
  return true;
}

void AuthService::cancelPending() {
  std::memset(pendingSetup_.password, 0, sizeof(pendingSetup_.password));
  pendingSetup_ = {};
  pendingReset_ = false;
  cancelHoldStartedMs_ = 0;
}

bool AuthService::beginResetRequest() {
  if (!credentialStore_.hasCredentials()) {
    return false;
  }
  if (pendingSetup_.active) {
    return false;
  }
  pendingReset_ = true;
  return true;
}

bool AuthService::confirmReset() {
  if (!pendingReset_) {
    return false;
  }
  credentialStore_.clear();
  sessions_.clear();
  pendingReset_ = false;
  cancelHoldStartedMs_ = 0;
  loginFailures_ = 0;
  loginLockUntilMs_ = 0;
  return true;
}

bool AuthService::login(const char* username, const char* password, char* sessionTokenOut,
                        size_t sessionTokenCapacity, char* error, size_t errorCapacity) {
  const uint32_t now = nowMs();
  if (now < loginLockUntilMs_) {
    util::copyCString(error, errorCapacity, "locked");
    return false;
  }
  if (!credentialStore_.hasCredentials()) {
    util::copyCString(error, errorCapacity, "not_configured");
    return false;
  }
  StoredCredentials credentials;
  if (!credentialStore_.load(credentials)) {
    util::copyCString(error, errorCapacity, "not_configured");
    return false;
  }

  // Bound-check untrusted input before any hashing work.
  char policyError[32] = {};
  if (!CredentialPolicy::validateUsername(username, policyError, sizeof(policyError)) ||
      !CredentialPolicy::validatePassword(password, policyError, sizeof(policyError))) {
    return recordLoginFailure(now, error, errorCapacity);
  }

  const bool userOk = std::strcmp(username, credentials.username) == 0;
  const bool passOk =
      userOk && PasswordHasher::verifyPassword(password, credentials.salt, credentials.hash);
  if (!passOk) {
    return recordLoginFailure(now, error, errorCapacity);
  }

  loginFailures_ = 0;
  loginLockUntilMs_ = 0;
  if (!sessions_.create(sessionTokenOut, sessionTokenCapacity, now)) {
    util::copyCString(error, errorCapacity, "session_failed");
    return false;
  }
  util::copyCString(error, errorCapacity, "");
  return true;
}

void AuthService::logout(const char* sessionToken) {
  sessions_.revoke(sessionToken);
}

bool AuthService::isAuthenticated(const char* sessionToken) const {
  return sessions_.valid(sessionToken, nowMs());
}

void AuthService::onCancelHeld(bool held, uint32_t nowMsValue) {
  if (pendingSetup_.active || pendingReset_) {
    cancelHoldStartedMs_ = 0;
    return;
  }
  if (!credentialStore_.hasCredentials()) {
    cancelHoldStartedMs_ = 0;
    return;
  }
  if (!held) {
    cancelHoldStartedMs_ = 0;
    return;
  }
  if (cancelHoldStartedMs_ == 0) {
    cancelHoldStartedMs_ = nowMsValue;
    return;
  }
  if ((nowMsValue - cancelHoldStartedMs_) >= kResetHoldMs) {
    beginResetRequest();
    cancelHoldStartedMs_ = 0;
  }
}

void AuthService::onConfirmPressed() {
  if (pendingSetup_.active) {
    confirmSetup();
  } else if (pendingReset_) {
    confirmReset();
  }
}

void AuthService::onCancelPressed() {
  cancelPending();
}

}  // namespace wattcycle::auth
