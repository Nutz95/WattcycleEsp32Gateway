#pragma once

#include "Auth/IAuthSessionService.h"
#include "Shelly/IShellySmokeHub.h"

#include <WebServer.h>

namespace wattcycle::web {

/// HTTP routes for Shelly Plus Smoke whitelist. Wired from EspWebGateway.
class ShellySmokeHttp {
 public:
  ShellySmokeHttp(WebServer& server, auth::IAuthSessionService& sessions);

  void setHub(shelly::IShellySmokeHub* smoke);
  void registerRoutes();

 private:
  void handleGet();
  void handleListen();
  void handleAuthorize();
  void handleRename();
  void handleRevoke();

  WebServer& server_;
  auth::IAuthSessionService& sessions_;
  shelly::IShellySmokeHub* smoke_ = nullptr;
};

}  // namespace wattcycle::web
