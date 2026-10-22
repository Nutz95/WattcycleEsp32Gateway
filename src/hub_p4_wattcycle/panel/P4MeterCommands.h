#pragma once

#include "EspNow/IEspNowCommandSender.h"

namespace wattcycle::panel {

/// Meter commands stay unanswered until the hosted ESP-NOW send path exists.
class P4MeterCommands : public espnow_rx::IEspNowCommandSender {
 public:
  /// Always refuses. The panel receives frames; it does not transmit meter resets.
  bool sendMeterCommand(uint8_t command) override;
};

}  // namespace wattcycle::panel
