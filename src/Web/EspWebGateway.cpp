#include "Web/EspWebGateway.h"

#ifndef UNIT_TEST

#include "Telemetry/TelemetryJsonSerializer.h"

#include <FS.h>
#include <LittleFS.h>

namespace wattcycle::web {

EspWebGateway::EspWebGateway(telemetry::ITelemetryStore& store) : store_(store) {}

bool EspWebGateway::begin(uint16_t port) {
  if (!LittleFS.begin(true)) {
    return false;
  }

  server_ = std::unique_ptr<WebServer>(new WebServer(port));
  server_->on("/", HTTP_GET, [this]() { handleRoot(); });
  server_->on("/api/telemetry", HTTP_GET, [this]() { handleApiTelemetry(); });
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

void EspWebGateway::handleRoot() {
  if (LittleFS.exists("/index.html")) {
    File file = LittleFS.open("/index.html", "r");
    server_->streamFile(file, "text/html");
    file.close();
    return;
  }
  server_->send(200, "text/plain", "Wattcycle Gateway — upload LittleFS data/");
}

void EspWebGateway::handleApiTelemetry() {
  char json[2048];
  const size_t written =
      telemetry::TelemetryJsonSerializer::serialize(store_, json, sizeof(json));
  if (written == 0) {
    server_->send(500, "application/json", "{\"error\":\"serialize_failed\"}");
    return;
  }
  server_->send(200, "application/json", json);
}

void EspWebGateway::handleNotFound() {
  const String path = server_->uri();
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
