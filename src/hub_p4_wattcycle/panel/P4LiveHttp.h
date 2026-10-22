#pragma once

#include "Auth/IAuthSessionService.h"

#include <WiFi.h>

namespace wattcycle::panel {

bool liveWriteAll(WiFiClient& client, const uint8_t* data, size_t length);
bool liveAcceptAuthorized(WiFiServer& server, WiFiClient& client, auth::IAuthSessionService& auth,
                          char* origin, size_t originCapacity);
void liveSendStatus(WiFiClient& client, int code, const char* reason);
void liveFillCors(const char* origin, char* out, size_t capacity);

}  // namespace wattcycle::panel
