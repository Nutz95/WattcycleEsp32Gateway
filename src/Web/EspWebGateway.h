#pragma once

#include "Telemetry/ITelemetryStore.h"
#include "Web/IWebGateway.h"

#ifndef UNIT_TEST
#include <WebServer.h>
#include <memory>
#endif

namespace wattcycle::web {

class EspWebGateway : public IWebGateway {
 public:
  explicit EspWebGateway(telemetry::ITelemetryStore& store);

  bool begin(uint16_t port) override;
  void loop() override;

 private:
  void handleRoot();
  void handleApiTelemetryBinary();
  void handleNotFound();
  bool trySendCached(const char* path, const char* contentType);
  void sendWithEtag(const char* path, const char* contentType);

  telemetry::ITelemetryStore& store_;
#ifndef UNIT_TEST
  std::unique_ptr<WebServer> server_;
#endif
  bool started_ = false;
  char indexEtag_[17] = {};
  char cssEtag_[17] = {};
  char jsEtag_[17] = {};
};

}  // namespace wattcycle::web
