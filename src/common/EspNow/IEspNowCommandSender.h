#pragma once

#include <cstdint>

namespace wattcycle::espnow_rx {

/// Minimal sink used by the web gateway for XT369P meter commands.
class IEspNowCommandSender {
 public:
  virtual ~IEspNowCommandSender() = default;
  /// Send one XT369P meter command byte to the peer bridge.
  virtual bool sendMeterCommand(uint8_t command) = 0;
};

}  // namespace wattcycle::espnow_rx
