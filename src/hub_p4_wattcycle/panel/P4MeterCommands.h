#pragma once

#include "EspNow/IEspNowCommandSender.h"

namespace wattcycle::panel {

/// Sends XT369P reset commands through the hosted coprocessor.
class P4MeterCommands : public espnow_rx::IEspNowCommandSender {
 public:
  /// Queue one meter command to the solar peer.
  bool sendMeterCommand(uint8_t command) override;
};

}  // namespace wattcycle::panel
