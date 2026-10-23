#include "HttpApi/EspWebGateway.h"

#include "HttpApi/HttpReadDecision.h"
#include "HttpApi/JsonField.h"

#include <cstdio>
#include <cstring>

namespace wattcycle::web {
namespace {

bool copyPlain(WebServer& server, char* body, size_t capacity) {
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

}  // namespace

void EspWebGateway::setGuestAccess(auth::IGuestAccess& guest) {
  guest_ = &guest;
}

void EspWebGateway::setMicGain(audio::IMicGain& gain) {
  micGain_ = &gain;
}

void EspWebGateway::setShellyPlugs(shelly::IShellyPlugHub& plugs,
                                   shelly::IShellyPlugHistoryStore* history) {
  plugsHub_ = &plugs;
  plugsHistory_ = history;
}

void EspWebGateway::handleGuest() {
  if (server_->method() == HTTP_GET) {
    char token[auth::kSessionTokenHexLen + 1] = {};
    collectSessionToken(token, sizeof(token));
    if (!decideHttpRead(guest_, authSessions_, token).allowed()) {
      sendJson(401, "{\"error\":\"unauthorized\"}");
      return;
    }
    const bool on = guest_ != nullptr && guest_->enabled();
    const unsigned long tabs = guest_ != nullptr ? guest_->tabs() : 0;
    char json[48] = {};
    std::snprintf(json, sizeof(json), "{\"enabled\":%s,\"tabs\":%lu}", on ? "true" : "false", tabs);
    sendJson(200, json);
    return;
  }
  if (server_->method() != HTTP_POST) {
    sendJson(405, "{\"error\":\"method\"}");
    return;
  }
  if (!requireAuth()) {
    return;
  }
  if (guest_ == nullptr) {
    sendJson(404, "{\"error\":\"unsupported\"}");
    return;
  }
  char body[96] = {};
  bool enabled = false;
  if (!copyPlain(*server_, body, sizeof(body)) || !extractJsonBoolField(body, "enabled", enabled)) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  int tabs = static_cast<int>(guest_->tabs());
  if (extractJsonIntField(body, "tabs", tabs) && tabs < 0) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  if (!guest_->update(enabled, static_cast<uint32_t>(tabs))) {
    sendJson(503, "{\"error\":\"sd_unavailable\"}");
    return;
  }
  char json[48] = {};
  std::snprintf(json, sizeof(json), "{\"enabled\":%s,\"tabs\":%lu}", enabled ? "true" : "false",
                static_cast<unsigned long>(guest_->tabs()));
  sendJson(200, json);
}

void EspWebGateway::handleMicGain() {
  if (micGain_ == nullptr) {
    sendJson(404, "{\"error\":\"unsupported\"}");
    return;
  }
  if (server_->method() == HTTP_GET) {
    if (!requireAuth()) {
      return;
    }
    char json[32] = {};
    std::snprintf(json, sizeof(json), "{\"percent\":%d}", micGain_->percent());
    sendJson(200, json);
    return;
  }
  if (server_->method() != HTTP_POST) {
    sendJson(405, "{\"error\":\"method\"}");
    return;
  }
  if (!requireAuth()) {
    return;
  }
  char body[64] = {};
  int percent = 0;
  if (!copyPlain(*server_, body, sizeof(body)) || !extractJsonIntField(body, "percent", percent)) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  micGain_->setPercent(percent);
  char json[32] = {};
  std::snprintf(json, sizeof(json), "{\"percent\":%d}", micGain_->percent());
  sendJson(200, json);
}

}  // namespace wattcycle::web
