#pragma once

#include "Auth/IAuthSessionService.h"
#include "Shelly/IShellyPlugHistoryStore.h"
#include "Shelly/IShellyPlugHub.h"

#include <WebServer.h>

namespace wattcycle::web {

/// HTTP routes for Shelly plug whitelist + day CSV. Wired from EspWebGateway.
class ShellyPlugsHttp {
 public:
  ShellyPlugsHttp(WebServer& server, auth::IAuthSessionService& sessions);

  void setHub(shelly::IShellyPlugHub* plugs);
  void setHistory(shelly::IShellyPlugHistoryStore* history);
  void registerRoutes();

 private:
  void handleGet();
  void handleListen();
  void handleAuthorize();
  void handleRename();
  void handleRevoke();
  void handleHistoryDay();

  WebServer& server_;
  auth::IAuthSessionService& sessions_;
  shelly::IShellyPlugHub* plugs_ = nullptr;
  shelly::IShellyPlugHistoryStore* history_ = nullptr;
};

}  // namespace wattcycle::web
