#pragma once

#include <cstdint>

namespace wattcycle::web {

/// HTTP server used by hubs that expose the dashboard.
class IWebGateway {
 public:
  virtual ~IWebGateway() = default;

  /// Bind and start serving on `port`.
  virtual bool begin(uint16_t port) = 0;
  /// Pump accepted clients once.
  virtual void loop() = 0;
};

}  // namespace wattcycle::web
