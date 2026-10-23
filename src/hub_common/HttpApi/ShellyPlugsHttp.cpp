#include "HttpApi/ShellyPlugsHttp.h"

#include "Auth/AuthTypes.h"
#include "HttpApi/JsonField.h"
#include "Shelly/ShellyPlugLimits.h"
#include "Shelly/ShellyPlugReading.h"
#include "Util/SafeCopy.h"

#include <Arduino.h>

#include <cstdio>
#include <cstring>

namespace wattcycle::web {
namespace {

bool copyPlainBody(WebServer& server, char* body, size_t capacity) {
  if (!server.hasArg("plain")) {
    return false;
  }
  const String plain = server.arg("plain");
  if (plain.length() == 0 || plain.length() >= capacity) {
    return false;
  }
  std::memcpy(body, plain.c_str(), plain.length());
  body[plain.length()] = '\0';
  return true;
}

void appendEscaped(char* out, size_t capacity, size_t& used, const char* text) {
  if (text == nullptr) {
    return;
  }
  for (const char* cursor = text; *cursor != '\0' && used + 2 < capacity; ++cursor) {
    const char ch = *cursor;
    if (ch == '"' || ch == '\\') {
      if (used + 3 >= capacity) {
        return;
      }
      out[used++] = '\\';
    }
    if (static_cast<unsigned char>(ch) < 32u) {
      continue;
    }
    out[used++] = ch;
  }
  out[used] = '\0';
}

void appendReadingJson(char* json, size_t capacity, size_t& used, const shelly::ShellyPlugReading& reading,
                       bool first) {
  if (used + 64 >= capacity) {
    return;
  }
  used += static_cast<size_t>(std::snprintf(json + used, capacity - used, "%s{\"id\":\"", first ? "" : ","));
  appendEscaped(json, capacity, used, reading.id);
  used += static_cast<size_t>(std::snprintf(json + used, capacity - used, "\",\"name\":\""));
  appendEscaped(json, capacity, used, reading.name);
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
};

bool beginDayStream(size_t totalBytes, void* user) {
  DayStreamCtx* ctx = static_cast<DayStreamCtx*>(user);
  ctx->server->setContentLength(totalBytes);
  ctx->server->send(200, "text/csv", "");
  return true;
}

bool writeDayChunk(const uint8_t* data, size_t length, void* user) {
  DayStreamCtx* ctx = static_cast<DayStreamCtx*>(user);
  ctx->server->sendContent(reinterpret_cast<const char*>(data), length);
  return true;
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

void ShellyPlugsHttp::sendJson(int code, const char* json) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(code, "application/json", json);
}

void ShellyPlugsHttp::collectSessionToken(char* out, size_t capacity) const {
  out[0] = '\0';
  if (!server_.hasHeader("Cookie")) {
    return;
  }
  const String cookie = server_.header("Cookie");
  const int idx = cookie.indexOf("wg_session=");
  if (idx < 0) {
    return;
  }
  int start = idx + 11;
  int end = cookie.indexOf(';', start);
  if (end < 0) {
    end = cookie.length();
  }
  util::copyCString(out, capacity, cookie.substring(start, end).c_str());
}

bool ShellyPlugsHttp::requireAuth() {
  char token[auth::kSessionTokenHexLen + 1] = {};
  collectSessionToken(token, sizeof(token));
  if (!sessions_.isAuthenticated(token)) {
    sendJson(401, "{\"error\":\"unauthorized\"}");
    return false;
  }
  return true;
}

void ShellyPlugsHttp::handleGet() {
  if (plugs_ == nullptr) {
    sendJson(404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!requireAuth()) {
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
  sendJson(200, json);
}

void ShellyPlugsHttp::handleListen() {
  if (plugs_ == nullptr) {
    sendJson(404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!requireAuth()) {
    return;
  }
  char body[64] = {};
  if (!copyPlainBody(server_, body, sizeof(body))) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  bool open = false;
  if (!extractJsonBoolField(body, "open", open)) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  plugs_->setListen(open, millis());
  sendJson(200, open ? "{\"listen\":true}" : "{\"listen\":false}");
}

void ShellyPlugsHttp::handleAuthorize() {
  if (plugs_ == nullptr) {
    sendJson(404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!requireAuth()) {
    return;
  }
  char body[128] = {};
  if (!copyPlainBody(server_, body, sizeof(body))) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  char id[shelly::kShellyIdCapacity] = {};
  char name[shelly::kShellyNameCapacity] = {};
  if (!extractJsonStringField(body, "id", id, sizeof(id))) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  extractJsonStringField(body, "name", name, sizeof(name));
  if (!plugs_->authorize(id, name[0] != '\0' ? name : nullptr)) {
    sendJson(400, "{\"error\":\"authorize_failed\"}");
    return;
  }
  char stored[shelly::kShellyNameCapacity] = {};
  plugs_->copyName(id, stored, sizeof(stored));
  char json[96] = {};
  std::snprintf(json, sizeof(json), "{\"ok\":true,\"name\":\"");
  size_t used = std::strlen(json);
  appendEscaped(json, sizeof(json), used, stored);
  if (used + 3 < sizeof(json)) {
    std::memcpy(json + used, "\"}", 3);
  }
  sendJson(200, json);
}

void ShellyPlugsHttp::handleRename() {
  if (plugs_ == nullptr) {
    sendJson(404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!requireAuth()) {
    return;
  }
  char body[128] = {};
  if (!copyPlainBody(server_, body, sizeof(body))) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  char id[shelly::kShellyIdCapacity] = {};
  char name[shelly::kShellyNameCapacity] = {};
  if (!extractJsonStringField(body, "id", id, sizeof(id)) ||
      !extractJsonStringField(body, "name", name, sizeof(name))) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  if (!plugs_->rename(id, name)) {
    sendJson(400, "{\"error\":\"rename_failed\"}");
    return;
  }
  char stored[shelly::kShellyNameCapacity] = {};
  plugs_->copyName(id, stored, sizeof(stored));
  char json[96] = {};
  std::snprintf(json, sizeof(json), "{\"ok\":true,\"name\":\"");
  size_t used = std::strlen(json);
  appendEscaped(json, sizeof(json), used, stored);
  if (used + 3 < sizeof(json)) {
    std::memcpy(json + used, "\"}", 3);
  }
  sendJson(200, json);
}

void ShellyPlugsHttp::handleRevoke() {
  if (plugs_ == nullptr) {
    sendJson(404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!requireAuth()) {
    return;
  }
  char body[96] = {};
  if (!copyPlainBody(server_, body, sizeof(body))) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  char id[shelly::kShellyIdCapacity] = {};
  if (!extractJsonStringField(body, "id", id, sizeof(id))) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  if (!plugs_->revoke(id)) {
    sendJson(400, "{\"error\":\"revoke_failed\"}");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

void ShellyPlugsHttp::handleHistoryDay() {
  if (!requireAuth()) {
    return;
  }
  if (history_ == nullptr) {
    sendJson(404, "{\"error\":\"unavailable\"}");
    return;
  }
  if (!server_.hasArg("date") || !dateLooksUtc(server_.arg("date"))) {
    sendJson(400, "{\"error\":\"missing_date\"}");
    return;
  }
  DayStreamCtx ctx = {};
  ctx.server = &server_;
  if (!history_->streamDay(server_.arg("date").c_str(), &beginDayStream, &writeDayChunk, &ctx)) {
    sendJson(404, "{\"error\":\"day_not_found\"}");
  }
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
