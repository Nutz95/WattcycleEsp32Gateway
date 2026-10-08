#pragma once

#include <cstdint>

namespace wattcycle::web {

class IWebGateway {
 public:
  virtual ~IWebGateway() = default;

  virtual bool begin(uint16_t port) = 0;
  virtual void loop() = 0;
};

}  // namespace wattcycle::web
