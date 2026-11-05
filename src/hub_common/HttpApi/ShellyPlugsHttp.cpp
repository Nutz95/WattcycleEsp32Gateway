#include "HttpApi/ShellyPlugsHttp.h"

#include "HttpApi/GzipDayBody.h"
#include "HttpApi/JsonField.h"
#include "HttpApi/ShellyHttpSupport.h"
#include "Shelly/ShellyPlugLimits.h"
#include "Shelly/ShellyPlugReading.h"

#include <Arduino.h>

#include <cstdio>
#include <cstring>

namespace wattcycle::web {
namespace {

void appendReadingJson(char* json, size_t capacity, size_t& used, const shelly::ShellyPlugReading& reading,
                       bool first) {
  if (used + 64 >= capacity) {
    return;
  }
  used += static_cast<size_t>(std::snprintf(json + used, capacity - used, "%s{\"id\":\"", first ? "" : ","));
  ShellyHttpSupport::appendEscaped(json, capacity, used, reading.id);
  used += static_cast<size_t>(std::snprintf(json + used, capacity - used, "\",\"name\":\""));
  ShellyHttpSupport::appendEscaped(json, capacity, used, reading.name);
  used += static_cast<size_t>(std::snprintf(
      json + used, capacity - used,
      "\",\"watts\":%.1f,\"volts\":%.1f,\"amps\":%.3f,\"whTotal\":%.3f,\"whToday\":%.3f,"
      "\"output\":%s,\"online\":%s,\"authorized\":%s}",
      reading.watts, reading.volts, reading.amps, reading.whTotal, reading.whToday,
      reading.output ? "true" : "false", reading.online ? "true" : "false",
      reading.authorized ? "true" : "false"));
}

bool dateLooksUtc(const String& date) {
  return date.length() == 10 && date.charAt(4) == '-' && date.charAt(7) == '-';
}

struct DayStreamCtx {
  WebServer* server = nullptr;
  bool headerSent = false;
};

bool beginDayStream(size_t totalBytes, void* user) {
  auto* ctx = static_cast<DayStreamCtx*>(user);
  if (ctx == nullptr || ctx->server == nullptr) {
    return false;
  }
  if (totalBytes > 0) {
    ctx->server->setContentLength(totalBytes);
  } else {
    ctx->server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  }
  ctx->server->send(200, "text/csv; charset=utf-8", "");
  ctx->headerSent = true;
  return true;
}

bool writeDayChunk(const uint8_t* data, size_t length, void* user) {
  auto* ctx = static_cast<DayStreamCtx*>(user);
  if (ctx == nullptr || ctx->server == nullptr || data == nullptr) {
    return false;
  }
  if (length == 0) {
    return true;
  }
  return ctx->server->client().write(data, length) == length;
}

}  // namespace

ShellyPlugsHttp::ShellyPlugsHttp(WebServer& server, auth::IAuthSessionService& sessions)
    : server_(server), sessions_(sessions) {}

void ShellyPlugsHttp::setHub(shelly::IShellyPlugHub* plugs) {
  plugs_ = plugs;
}

void ShellyPlugsHttp::setHistory(shelly::IShellyPlugHistoryStore* history) {
  history_ = history;
}

void ShellyPlugsHttp::handleGet() {
  if (plugs_ == nullptr) {
    ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!ShellyHttpSupport::requireAuth(server_, sessions_)) {
    return;
  }
  const uint32_t nowMs = millis();
  shelly::ShellyPlugReading authorized[shelly::kMaxShellyPlugs] = {};
  shelly::ShellyPlugReading posting[shelly::kMaxShellyPlugs] = {};
  const size_t authCount = plugs_->snapshotAuthorized(authorized, shelly::kMaxShellyPlugs, nowMs);
  const size_t postCount = plugs_->snapshotPosting(posting, shelly::kMaxShellyPlugs, nowMs);
  char json[3200] = {};
  size_t used = 0;
  used += static_cast<size_t>(
      std::snprintf(json + used, sizeof(json) - used, "{\"listen\":%s,\"authorized\":[",
                    plugs_->listenOpen() ? "true" : "false"));
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

void ShellyPlugsHttp::handleListen() {
  if (plugs_ == nullptr) {
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
  plugs_->setListen(open, millis());
  ShellyHttpSupport::sendJson(server_, 200, open ? "{\"listen\":true}" : "{\"listen\":false}");
}

void ShellyPlugsHttp::handleAuthorize() {
  if (plugs_ == nullptr) {
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
  if (!plugs_->authorize(id, name[0] != '\0' ? name : nullptr)) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"authorize_failed\"}");
    return;
  }
  char stored[shelly::kShellyNameCapacity] = {};
  plugs_->copyName(id, stored, sizeof(stored));
  char json[96] = {};
  std::snprintf(json, sizeof(json), "{\"ok\":true,\"name\":\"");
  size_t used = std::strlen(json);
  ShellyHttpSupport::appendEscaped(json, sizeof(json), used, stored);
  if (used + 3 < sizeof(json)) {
    std::memcpy(json + used, "\"}", 3);
  }
  ShellyHttpSupport::sendJson(server_, 200, json);
}

void ShellyPlugsHttp::handleRename() {
  if (plugs_ == nullptr) {
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
  if (!plugs_->rename(id, name)) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"rename_failed\"}");
    return;
  }
  char stored[shelly::kShellyNameCapacity] = {};
  plugs_->copyName(id, stored, sizeof(stored));
  char json[96] = {};
  std::snprintf(json, sizeof(json), "{\"ok\":true,\"name\":\"");
  size_t used = std::strlen(json);
  ShellyHttpSupport::appendEscaped(json, sizeof(json), used, stored);
  if (used + 3 < sizeof(json)) {
    std::memcpy(json + used, "\"}", 3);
  }
  ShellyHttpSupport::sendJson(server_, 200, json);
}

void ShellyPlugsHttp::handleRevoke() {
  if (plugs_ == nullptr) {
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
  if (!plugs_->revoke(id)) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"revoke_failed\"}");
    return;
  }
  ShellyHttpSupport::sendJson(server_, 200, "{\"ok\":true}");
}

void ShellyPlugsHttp::handleHistoryDay() {
  if (!ShellyHttpSupport::requireAuth(server_, sessions_)) {
    return;
  }
  if (history_ == nullptr) {
    ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!server_.hasArg("date") || !dateLooksUtc(server_.arg("date"))) {
    ShellyHttpSupport::sendJson(server_, 400, "{\"error\":\"missing_date\"}");
    return;
  }
  const bool gzip = GzipDayBody::wantsGzip(server_);
  if (!gzip) {
    DayStreamCtx ctx = {};
    ctx.server = &server_;
    if (!history_->streamDay(server_.arg("date").c_str(), &beginDayStream, &writeDayChunk, &ctx)) {
      ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"day_not_found\"}");
    }
    return;
  }
  DayCsvBuffer buffer;
  if (!history_->streamDay(server_.arg("date").c_str(), &GzipDayBody::beginCollect,
                           &GzipDayBody::writeCollect, &buffer)) {
    ShellyHttpSupport::sendJson(server_, 404, "{\"error\":\"day_not_found\"}");
    return;
  }
  GzipDayBody::send(server_, buffer.data(), buffer.size(), true);
}

void ShellyPlugsHttp::registerRoutes() {
  server_.on("/api/plugs", HTTP_GET, [this]() { handleGet(); });
  server_.on("/api/plugs/listen", HTTP_POST, [this]() { handleListen(); });
  server_.on("/api/plugs/authorize", HTTP_POST, [this]() { handleAuthorize(); });
  server_.on("/api/plugs/rename", HTTP_POST, [this]() { handleRename(); });
  server_.on("/api/plugs/revoke", HTTP_POST, [this]() { handleRevoke(); });
  server_.on("/api/history/plugs/day", HTTP_GET, [this]() { handleHistoryDay(); });
}

}  // namespace wattcycle::web
