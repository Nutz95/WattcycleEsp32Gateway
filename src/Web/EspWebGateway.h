#pragma once

#include "Auth/IAuthService.h"
#include "Telemetry/ITelemetryStore.h"
#include "Web/IWebGateway.h"

#ifndef UNIT_TEST
#include <WebServer.h>
#include <memory>
#endif

namespace wattcycle::web {

class EspWebGateway : public IWebGateway {
 public:
  EspWebGateway(telemetry::ITelemetryStore& store, auth::IAuthService& authService);

  bool begin(uint16_t port) override;
  void loop() override;

 private:
  void handleRoot();
  void handleApiTelemetryBinary();
  void handleAuthStatus();
  void handleAuthSetup();
  void handleAuthLogin();
  void handleAuthLogout();
  void handleNotFound();
  bool trySendCached(const char* path, const char* contentType);
  void sendWithEtag(const char* path, const char* contentType);
  void collectSessionToken(char* out, size_t capacity) const;
  bool readAuthBody(char* body, size_t capacity, size_t& length, char* error,
                    size_t errorCapacity);
  /// Reads body + extracts username/password; sends 400 and returns false on failure.
  bool parseAuthCredentials(char* username, size_t usernameCapacity, char* password,
                            size_t passwordCapacity);
  void sendJson(int code, const char* json);
  void sendUnauthorized();
  bool requireAuth();

  telemetry::ITelemetryStore& store_;
  auth::IAuthService& authService_;
#ifndef UNIT_TEST
  std::unique_ptr<WebServer> server_;
#endif
  bool started_ = false;
  char indexEtag_[17] = {};
  char cssEtag_[17] = {};
  char jsEtag_[17] = {};
};

}  // namespace wattcycle::web
