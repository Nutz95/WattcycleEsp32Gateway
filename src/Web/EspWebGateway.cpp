#include "Web/EspWebGateway.h"

#ifndef UNIT_TEST

#include "Telemetry/BinaryTelemetryCodec.h"

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

EspWebGateway::EspWebGateway(telemetry::ITelemetryStore& store) : store_(store) {}

bool EspWebGateway::begin(uint16_t port) {
  if (!LittleFS.begin(true)) {
    return false;
  }

  computeFileEtag("/index.html", indexEtag_, sizeof(indexEtag_));
  computeFileEtag("/app.css", cssEtag_, sizeof(cssEtag_));
  computeFileEtag("/app.js", jsEtag_, sizeof(jsEtag_));

  server_ = std::unique_ptr<WebServer>(new WebServer(port));
  const char* collectHeaders[] = {"If-None-Match"};
  server_->collectHeaders(collectHeaders, 1);
  server_->on("/", HTTP_GET, [this]() { handleRoot(); });
  server_->on("/api/telemetry", HTTP_GET, [this]() { handleApiTelemetryBinary(); });
  server_->on("/api/telemetry.bin", HTTP_GET, [this]() { handleApiTelemetryBinary(); });
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

  if (server_->hasHeader("If-None-Match") &&
      server_->header("If-None-Match") == String(etag)) {
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

void EspWebGateway::handleApiTelemetryBinary() {
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
