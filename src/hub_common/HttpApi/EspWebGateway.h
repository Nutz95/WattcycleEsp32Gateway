#pragma once

#include "Audio/IMicGain.h"
#include "Auth/IAuthSessionService.h"
#include "Auth/IGuestAccess.h"
#include "EspNow/IEspNowCommandSender.h"
#include "Storage/IDailyHistoryStore.h"
#include "Telemetry/ITelemetryStore.h"
#include "HttpApi/IWebGateway.h"

#include <FS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <memory>

namespace wattcycle::web {

class EspWebGateway : public IWebGateway {
 public:
  EspWebGateway(telemetry::ITelemetryStore& store, auth::IAuthSessionService& authSessions,
                espnow_rx::IEspNowCommandSender& espNow,
                storage::IDailyHistoryStore& historyStore);

  bool begin(uint16_t port) override;
  void loop() override;
  /// role e.g. "m5_hub" / "ttgo_hub"; MACs may be empty until Wi-Fi is up.
  void setDeviceIdentity(const char* role, const char* hubStaMac, const char* bmsBridgeMac,
                         const char* xtBridgeMac, const char* ecoflowBridgeMac = "");
  /// Serve the site from this volume and leave LittleFS unmounted. Call before begin().
  void useFiles(fs::FS& volume);
  /// Optional. When unset, guest reads stay closed.
  void setGuestAccess(auth::IGuestAccess& guest);
  /// Optional. When unset, the mic-gain route answers 404.
  void setMicGain(audio::IMicGain& gain);

 private:
  void handleRoot();
  void handleApiTelemetryBinary();
  void handleSolarCommand();
  void handleHistoryDays();
  void handleHistoryDay();
  void handleDeviceInfo();
  void handleAuthStatus();
  void handleAuthSetup();
  void handleAuthLogin();
  void handleAuthLogout();
  void handleGuest();
  void handleMicGain();
  void handleNotFound();
  bool trySendCached(const char* path, const char* contentType);
  void sendWithEtag(const char* path, const char* contentType);
  void collectSessionToken(char* out, size_t capacity) const;
  bool readAuthBody(char* body, size_t capacity, size_t& length, char* error,
                    size_t errorCapacity);
  bool parseAuthCredentials(char* username, size_t usernameCapacity, char* password,
                            size_t passwordCapacity);
  void sendJson(int code, const char* json);
  void sendUnauthorized();
  bool requireAuth();
  bool permitRead();
  fs::FS& files();

  telemetry::ITelemetryStore& store_;
  auth::IAuthSessionService& authSessions_;
  espnow_rx::IEspNowCommandSender& espNow_;
  storage::IDailyHistoryStore& historyStore_;
  auth::IGuestAccess* guest_ = nullptr;
  audio::IMicGain* micGain_ = nullptr;
  char role_[24] = {};
  char hubStaMac_[18] = {};
  char bmsBridgeMac_[18] = {};
  char xtBridgeMac_[18] = {};
  char ecoflowBridgeMac_[18] = {};
  std::unique_ptr<WebServer> server_;
  fs::FS* files_ = nullptr;
  bool started_ = false;
  char indexEtag_[17] = {};
  char cssEtag_[17] = {};
  char jsEtag_[17] = {};
};

}  // namespace wattcycle::web
