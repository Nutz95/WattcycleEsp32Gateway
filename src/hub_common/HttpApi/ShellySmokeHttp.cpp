#include "HttpApi/ShellySmokeHttp.h"

#include "HttpApi/JsonField.h"
#include "HttpApi/ShellyHttpSupport.h"
#include "Shelly/ShellySmokeLimits.h"
#include "Shelly/ShellySmokeReading.h"

#include <Arduino.h>

#include <cstdio>
#include <cstring>

namespace wattcycle::web {
namespace {

void appendReadingJson(char* json, size_t capacity, size_t& used,
                       const shelly::ShellySmokeReading& reading, bool first) {
  if (used + 64 >= capacity) {
    return;
  }
  used += static_cast<size_t>(std::snprintf(json + used, capacity - used, "%s{\"id\":\"",
                                            first ? "" : ","));
  ShellyHttpSupport::appendEscaped(json, capacity, used, reading.id);
  used += static_cast<size_t>(std::snprintf(json + used, capacity - used, "\",\"name\":\""));
  ShellyHttpSupport::appendEscaped(json, capacity, used, reading.name);
  used += static_cast<size_t>(std::snprintf(
      json + used, capacity - used,
      "\",\"alarm\":%s,\"mute\":%s,\"batteryPercent\":%.0f,\"batteryVolts\":%.2f,"
      "\"online\":%s,\"authorized\":%s,\"lastMs\":%lu}",
      reading.alarm ? "true" : "false", reading.mute ? "true" : "false", reading.batteryPercent,
      reading.batteryVolts, reading.online ? "true" : "false",
      reading.authorized ? "true" : "false", static_cast<unsigned long>(reading.lastMs)));
}

}  // namespace

ShellySmokeHttp::ShellySmokeHttp(WebServer& server, auth::IAuthSessionService& sessions)
    : server_(server), sessions_(sessions) {}

void ShellySmokeHttp::setHub(shelly::IShellySmokeHub* smoke) {
  smoke_ = smoke;
}

void ShellySmokeHttp::handleGet() {
  if (smoke_ == nullptr) {
    ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!ShellyHttpSupport::requireAuth(server_, sessions_)) {
    return;
  }
  const uint32_t nowMs = millis();
  shelly::ShellySmokeReading authorized[shelly::kMaxShellySmoke] = {};
  shelly::ShellySmokeReading posting[shelly::kMaxShellySmoke] = {};
  const size_t authCount = smoke_->snapshotAuthorized(authorized, shelly::kMaxShellySmoke, nowMs);
  const size_t postCount = smoke_->snapshotPosting(posting, shelly::kMaxShellySmoke, nowMs);
  char json[2400] = {};
  size_t used = 0;
  used += static_cast<size_t>(
      std::snprintf(json + used, sizeof(json) - used, "{\"listen\":%s,\"authorized\":[",
                    smoke_->listenOpen() ? "true" : "false"));
  for (size_t i = 0; i < authCount; ++i) {
    appendReadingJson(json, sizeof(json), used, authorized[i], i == 0);
  }
  used += static_cast<size_t>(std::snprintf(json + used, sizeof(json) - used, "],\"posting\":["));
  for (size_t i = 0; i < postCount; ++i) {
    appendReadingJson(json, sizeof(json), used, posting[i], i == 0);
  }
  if (used + 3 < sizeof(json)) {
    std::memcpy(json + used, "]}", 3);
  }
  ShellyHttpSupport::sendJson(server_, 200, json);
}

void ShellySmokeHttp::handleListen() {
  if (smoke_ == nullptr) {
    ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!ShellyHttpSupport::requireAuth(server_, sessions_)) {
    return;
  }
  char body[64] = {};
  if (!ShellyHttpSupport::copyPlainBody(server_, body, sizeof(body))) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"invalid_json\"}");
    return;
  }
  bool open = false;
  if (!extractJsonBoolField(body, "open", open)) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"invalid_json\"}");
    return;
  }
  smoke_->setListen(open, millis());
  ShellyHttpSupport::sendJson(server_, 200, open ? "{\"listen\":true}" : "{\"listen\":false}");
}

void ShellySmokeHttp::handleAuthorize() {
  if (smoke_ == nullptr) {
    ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!ShellyHttpSupport::requireAuth(server_, sessions_)) {
    return;
  }
  char body[128] = {};
  if (!ShellyHttpSupport::copyPlainBody(server_, body, sizeof(body))) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"invalid_json\"}");
    return;
  }
  char id[shelly::kShellyIdCapacity] = {};
  char name[shelly::kShellyNameCapacity] = {};
  if (!extractJsonStringField(body, "id", id, sizeof(id))) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"invalid_json\"}");
    return;
  }
  extractJsonStringField(body, "name", name, sizeof(name));
  if (!smoke_->authorize(id, name[0] != '\0' ? name : nullptr)) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"authorize_failed\"}");
    return;
  }
  char stored[shelly::kShellyNameCapacity] = {};
  smoke_->copyName(id, stored, sizeof(stored));
  char json[96] = {};
  std::snprintf(json, sizeof(json), "{\"ok\":true,\"name\":\"");
  size_t used = std::strlen(json);
  ShellyHttpSupport::appendEscaped(json, sizeof(json), used, stored);
  if (used + 3 < sizeof(json)) {
    std::memcpy(json + used, "\"}", 3);
  }
  ShellyHttpSupport::sendJson(server_, 200, json);
}

void ShellySmokeHttp::handleRename() {
  if (smoke_ == nullptr) {
    ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!ShellyHttpSupport::requireAuth(server_, sessions_)) {
    return;
  }
  char body[128] = {};
  if (!ShellyHttpSupport::copyPlainBody(server_, body, sizeof(body))) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"invalid_json\"}");
    return;
  }
  char id[shelly::kShellyIdCapacity] = {};
  char name[shelly::kShellyNameCapacity] = {};
  if (!extractJsonStringField(body, "id", id, sizeof(id)) ||
      !extractJsonStringField(body, "name", name, sizeof(name))) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"invalid_json\"}");
    return;
  }
  if (!smoke_->rename(id, name)) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"rename_failed\"}");
    return;
  }
  char stored[shelly::kShellyNameCapacity] = {};
  smoke_->copyName(id, stored, sizeof(stored));
  char json[96] = {};
  std::snprintf(json, sizeof(json), "{\"ok\":true,\"name\":\"");
  size_t used = std::strlen(json);
  ShellyHttpSupport::appendEscaped(json, sizeof(json), used, stored);
  if (used + 3 < sizeof(json)) {
    std::memcpy(json + used, "\"}", 3);
  }
  ShellyHttpSupport::sendJson(server_, 200, json);
}

void ShellySmokeHttp::handleRevoke() {
  if (smoke_ == nullptr) {
    ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!ShellyHttpSupport::requireAuth(server_, sessions_)) {
    return;
  }
  char body[96] = {};
  if (!ShellyHttpSupport::copyPlainBody(server_, body, sizeof(body))) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"invalid_json\"}");
    return;
  }
  char id[shelly::kShellyIdCapacity] = {};
  if (!extractJsonStringField(body, "id", id, sizeof(id))) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"invalid_json\"}");
    return;
  }
  if (!smoke_->revoke(id)) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"revoke_failed\"}");
    return;
  }
  ShellyHttpSupport::sendJson(server_, 200, "{\"ok\":true}");
}

void ShellySmokeHttp::registerRoutes() {
  server_.on("/api/smoke", HTTP_GET, [this]() { handleGet(); });
  server_.on("/api/smoke/listen", HTTP_POST, [this]() { handleListen(); });
  server_.on("/api/smoke/authorize", HTTP_POST, [this]() { handleAuthorize(); });
  server_.on("/api/smoke/rename", HTTP_POST, [this]() { handleRename(); });
  server_.on("/api/smoke/revoke", HTTP_POST, [this]() { handleRevoke(); });
}

}  // namespace wattcycle::web
