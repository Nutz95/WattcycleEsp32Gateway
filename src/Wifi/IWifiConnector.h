#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::wifi {

class IWifiConnector {
 public:
  virtual ~IWifiConnector() = default;

  virtual bool connect(const char* ssid, const char* password,
                       uint32_t timeoutMs) = 0;
  virtual bool isConnected() const = 0;
  virtual void copyIpAddress(char* buffer, size_t capacity) const = 0;
  virtual void loop() = 0;
};

}  // namespace wattcycle::wifi
