#include "P4MeterCommands.h"

#include "EspNow/Xt369pEspNowProtocol.h"
#include "P4BuildSecrets.h"
#include "P4HostedNow.h"
#include "Util/MacAddress.h"

namespace wattcycle::panel {

bool P4MeterCommands::sendMeterCommand(uint8_t command) {
  uint8_t mac[6] = {};
  if (!wattcycle::util::parseMacAddress(P4_SOLAR_MAC, mac)) {
    return false;
  }
  xt369p_bridge::EspNowCommandV1 packet = {};
  packet.magic = xt369p_bridge::kCmdMagic;
  packet.version = xt369p_bridge::kVersion;
  packet.command = command;
  return hostedNowSend(mac, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet)) == ESP_OK;
}

}  // namespace wattcycle::panel
