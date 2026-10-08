#include "Web/EspWebGateway.h"

#ifndef UNIT_TEST

#include "Auth/CredentialPolicy.h"
#include "Telemetry/BinaryTelemetryCodec.h"
#include "Util/SafeCopy.h"
#include "Web/JsonField.h"

#include <FS.h>
#include <LittleFS.h>
#include <cstdio>
#include <cstring>

namespace wattcycle::web {
namespace {

uint32_t fnv1aFile(File& file) {
  uint32_t hash = 2166136261u;
  while (file.available()) {
    hash ^= static_cast<uint32_t>(file.read());
    hash *= 16777619u;
  }
  return hash;
}

void formatEtag(uint32_t hash, char* out, size_t capacity) {
  std::snprintf(out, capacity, "\"%08x\"", static_cast<unsigned>(hash));
}

bool computeFileEtag(const char* path, char* out, size_t capacity) {
  if (!LittleFS.exists(path)) {
    out[0] = '\0';
    return false;
  }
  File file = LittleFS.open(path, "r");
  if (!file) {
    out[0] = '\0';
    return false;
  }
  formatEtag(fnv1aFile(file), out, capacity);
  file.close();
  return true;
}

}  // namespace

EspWebGateway::EspWebGateway(telemetry::ITelemetryStore& store,
                             auth::IAuthSessionService& authSessions,
                             espnow_rx::IEspNowTelemetryReceiver& espNow)
    : store_(store), authSessions_(authSessions), espNow_(espNow) {}

bool EspWebGateway::begin(uint16_t port) {
  if (!LittleFS.begin(true)) {
    return false;
  }

  computeFileEtag("/index.html", indexEtag_, sizeof(indexEtag_));
  computeFileEtag("/app.css", cssEtag_, sizeof(cssEtag_));
  computeFileEtag("/app.js", jsEtag_, sizeof(jsEtag_));

  server_ = std::unique_ptr<WebServer>(new WebServer(port));
  const char* collectHeaders[] = {"If-None-Match", "Cookie"};
  server_->collectHeaders(collectHeaders, 2);
  server_->on("/", HTTP_GET, [this]() { handleRoot(); });
  server_->on("/api/auth/status", HTTP_GET, [this]() { handleAuthStatus(); });
  server_->on("/api/auth/setup", HTTP_POST, [this]() { handleAuthSetup(); });
  server_->on("/api/auth/login", HTTP_POST, [this]() { handleAuthLogin(); });
  server_->on("/api/auth/logout", HTTP_POST, [this]() { handleAuthLogout(); });
  server_->on("/api/telemetry", HTTP_GET, [this]() { handleApiTelemetryBinary(); });
  server_->on("/api/telemetry.bin", HTTP_GET, [this]() { handleApiTelemetryBinary(); });
  server_->on("/api/solar/command", HTTP_POST, [this]() { handleSolarCommand(); });
  server_->onNotFound([this]() { handleNotFound(); });
  server_->begin();
  started_ = true;
  return true;
}

void EspWebGateway::loop() {
  if (started_ && server_) {
    server_->handleClient();
  }
}

void EspWebGateway::collectSessionToken(char* out, size_t capacity) const {
  out[0] = '\0';
  if (!server_->hasHeader("Cookie")) {
    return;
  }
  const String cookie = server_->header("Cookie");
  const int idx = cookie.indexOf("wg_session=");
  if (idx < 0) {
    return;
  }
  int start = idx + 11;
  int end = cookie.indexOf(';', start);
  if (end < 0) {
    end = cookie.length();
  }
  wattcycle::util::copyCString(out, capacity, cookie.substring(start, end).c_str());
}

bool EspWebGateway::readAuthBody(char* body, size_t capacity, size_t& length, char* error,
                                 size_t errorCapacity) {
  length = 0;
  if (!server_->hasArg("plain")) {
    wattcycle::util::copyCString(error, errorCapacity, "invalid_json");
    return false;
  }
  const String plain = server_->arg("plain");
  const size_t contentLength = plain.length();
  if (contentLength == 0 || contentLength > auth::CredentialPolicy::kMaxJsonBodyBytes ||
      contentLength >= capacity) {
    wattcycle::util::copyCString(error, errorCapacity,
                                 contentLength > auth::CredentialPolicy::kMaxJsonBodyBytes
                                     ? "payload_too_large"
                                     : "invalid_json");
    return false;
  }
  std::memcpy(body, plain.c_str(), contentLength);
  body[contentLength] = '\0';
  length = contentLength;
  return auth::CredentialPolicy::validateAuthJsonBody(body, length, error, errorCapacity);
}

void EspWebGateway::sendJson(int code, const char* json) {
  server_->sendHeader("Cache-Control", "no-store");
  server_->send(code, "application/json", json);
}

void EspWebGateway::sendUnauthorized() {
  sendJson(401, "{\"error\":\"unauthorized\"}");
}

bool EspWebGateway::requireAuth() {
  char token[auth::kSessionTokenHexLen + 1] = {};
  collectSessionToken(token, sizeof(token));
  if (!authSessions_.isAuthenticated(token)) {
    sendUnauthorized();
    return false;
  }
  return true;
}

bool EspWebGateway::trySendCached(const char* path, const char* contentType) {
  const char* etag = nullptr;
  if (std::strcmp(path, "/index.html") == 0) {
    etag = indexEtag_;
  } else if (std::strcmp(path, "/app.css") == 0) {
    etag = cssEtag_;
  } else if (std::strcmp(path, "/app.js") == 0) {
    etag = jsEtag_;
  }

  if (etag == nullptr || etag[0] == '\0') {
    return false;
  }

  if (server_->hasHeader("If-None-Match") && server_->header("If-None-Match") == String(etag)) {
    server_->send(304, contentType, "");
    return true;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    return false;
  }
  server_->sendHeader("ETag", etag);
  server_->sendHeader("Cache-Control", "public, max-age=60");
  server_->streamFile(file, contentType);
  file.close();
  return true;
}

void EspWebGateway::sendWithEtag(const char* path, const char* contentType) {
  if (trySendCached(path, contentType)) {
    return;
  }
  if (!LittleFS.exists(path)) {
    server_->send(404, "text/plain", "Not found");
    return;
  }
  File file = LittleFS.open(path, "r");
  server_->streamFile(file, contentType);
  file.close();
}

void EspWebGateway::handleRoot() {
  sendWithEtag("/index.html", "text/html");
}

void EspWebGateway::handleAuthStatus() {
  char token[auth::kSessionTokenHexLen + 1] = {};
  collectSessionToken(token, sizeof(token));
  const auto st = authSessions_.status(token);
  char json[160];
  std::snprintf(json, sizeof(json),
                "{\"configured\":%s,\"pendingSetup\":%s,\"pendingReset\":%s,\"authenticated\":%s}",
                st.configured ? "true" : "false", st.pendingSetup ? "true" : "false",
                st.pendingReset ? "true" : "false", st.authenticated ? "true" : "false");
  sendJson(200, json);
}

bool EspWebGateway::parseAuthCredentials(char* username, size_t usernameCapacity, char* password,
                                         size_t passwordCapacity) {
  char body[auth::CredentialPolicy::kMaxJsonBodyBytes + 1] = {};
  size_t length = 0;
  char error[32] = {};
  if (!readAuthBody(body, sizeof(body), length, error, sizeof(error))) {
    char json[80];
    std::snprintf(json, sizeof(json), "{\"error\":\"%s\"}", error[0] ? error : "invalid_json");
    sendJson(400, json);
    return false;
  }
  if (!extractJsonStringField(body, "username", username, usernameCapacity) ||
      !extractJsonStringField(body, "password", password, passwordCapacity)) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return false;
  }
  return true;
}

void EspWebGateway::handleAuthSetup() {
  char username[auth::kMaxUsernameLen + 1] = {};
  char password[auth::kMaxPasswordLen + 1] = {};
  if (!parseAuthCredentials(username, sizeof(username), password, sizeof(password))) {
    return;
  }
  char error[32] = {};
  if (!authSessions_.beginSetup(username, password, error, sizeof(error))) {
    char json[80];
    std::snprintf(json, sizeof(json), "{\"error\":\"%s\"}", error[0] ? error : "rejected");
    sendJson(400, json);
    return;
  }
  sendJson(202, "{\"ok\":true,\"needsPhysicalConfirm\":true}");
}

void EspWebGateway::handleAuthLogin() {
  char username[auth::kMaxUsernameLen + 1] = {};
  char password[auth::kMaxPasswordLen + 1] = {};
  if (!parseAuthCredentials(username, sizeof(username), password, sizeof(password))) {
    return;
  }
  char token[auth::kSessionTokenHexLen + 1] = {};
  char error[32] = {};
  if (!authSessions_.login(username, password, token, sizeof(token), error, sizeof(error))) {
    char json[80];
    std::snprintf(json, sizeof(json), "{\"error\":\"%s\"}", error[0] ? error : "rejected");
    const int code = (std::strcmp(error, "locked") == 0) ? 429 : 401;
    sendJson(code, json);
    return;
  }
  char cookie[96];
  std::snprintf(cookie, sizeof(cookie),
                "wg_session=%s; Path=/; HttpOnly; SameSite=Strict; Max-Age=86400", token);
  server_->sendHeader("Set-Cookie", cookie);
  sendJson(200, "{\"ok\":true}");
}

void EspWebGateway::handleAuthLogout() {
  char token[auth::kSessionTokenHexLen + 1] = {};
  collectSessionToken(token, sizeof(token));
  authSessions_.logout(token);
  server_->sendHeader("Set-Cookie", "wg_session=; Path=/; Max-Age=0");
  sendJson(200, "{\"ok\":true}");
}

void EspWebGateway::handleApiTelemetryBinary() {
  if (!requireAuth()) {
    return;
  }
  uint8_t payload[telemetry::BinaryTelemetryCodec::kMaxEncodedBytes];
  const size_t written =
      telemetry::BinaryTelemetryCodec::encode(store_, payload, sizeof(payload));
  if (written == 0) {
    server_->send(500, "text/plain", "encode_failed");
    return;
  }
  server_->sendHeader("Cache-Control", "no-store");
  server_->setContentLength(written);
  server_->send(200, "application/octet-stream", "");
  server_->sendContent(reinterpret_cast<const char*>(payload), written);
}

void EspWebGateway::handleSolarCommand() {
  if (!requireAuth()) {
    return;
  }
  char body[96] = {};
  size_t length = 0;
  char error[32] = {};
  if (!readAuthBody(body, sizeof(body), length, error, sizeof(error))) {
    sendJson(400, "{\"error\":\"invalid_json\"}");
    return;
  }
  char command[24] = {};
  if (!extractJsonStringField(body, "command", command, sizeof(command))) {
    sendJson(400, "{\"error\":\"missing_command\"}");
    return;
  }

  uint8_t wire = 0;
  if (std::strcmp(command, "reset_wh") == 0) {
    wire = 0x01;
  } else if (std::strcmp(command, "reset_ah") == 0) {
    wire = 0x02;
  } else if (std::strcmp(command, "reset_duration") == 0) {
    wire = 0x03;
  } else if (std::strcmp(command, "reset_all") == 0) {
    wire = 0x05;
  } else {
    sendJson(400, "{\"error\":\"unknown_command\"}");
    return;
  }

  if (!espNow_.sendMeterCommand(wire)) {
    sendJson(503, "{\"error\":\"espnow_send_failed\"}");
    return;
  }
  sendJson(200, "{\"ok\":true}");
}

void EspWebGateway::handleNotFound() {
  const String path = server_->uri();
  if (path == "/app.css") {
    sendWithEtag("/app.css", "text/css");
    return;
  }
  if (path == "/app.js") {
    sendWithEtag("/app.js", "application/javascript");
    return;
  }
  if (LittleFS.exists(path)) {
    String contentType = "text/plain";
    if (path.endsWith(".css")) {
      contentType = "text/css";
    } else if (path.endsWith(".js")) {
      contentType = "application/javascript";
    } else if (path.endsWith(".html")) {
      contentType = "text/html";
    }
    File file = LittleFS.open(path, "r");
    server_->streamFile(file, contentType);
    file.close();
    return;
  }
  server_->send(404, "text/plain", "Not found");
}

}  // namespace wattcycle::web

#endif
