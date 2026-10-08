#pragma once

#include "Auth/IAuthService.h"
#include "Auth/ICredentialStore.h"
#include "Auth/SessionStore.h"

namespace wattcycle::auth {

class AuthService : public IAuthService {
 public:
  explicit AuthService(ICredentialStore& credentialStore);

  void begin() override;
  AuthStatus status(const char* sessionToken) const override;
  AuthPrompt prompt() const override;

  bool beginSetup(const char* username, const char* password, char* error,
                  size_t errorCapacity) override;

  bool login(const char* username, const char* password, char* sessionTokenOut,
             size_t sessionTokenCapacity, char* error, size_t errorCapacity) override;
  void logout(const char* sessionToken) override;
  bool isAuthenticated(const char* sessionToken) const override;

  void onCancelHeld(bool held, uint32_t nowMs) override;
  void onConfirmPressed() override;
  void onCancelPressed() override;

 private:
  bool confirmSetup();
  void cancelPending();
  bool beginResetRequest();
  bool confirmReset();
  bool recordLoginFailure(uint32_t nowMs, char* error, size_t errorCapacity);
  uint32_t nowMs() const;

  bool hasStoredCredentials() const;

  ICredentialStore& credentialStore_;
  SessionStore sessions_{};
  PendingSetup pendingSetup_{};
  bool pendingReset_ = false;
  bool configured_ = false;
  uint32_t cancelHoldStartedMs_ = 0;
  uint32_t loginFailures_ = 0;
  uint32_t loginLockUntilMs_ = 0;
};

}  // namespace wattcycle::auth
