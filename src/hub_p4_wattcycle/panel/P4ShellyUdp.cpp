#include "P4ShellyUdp.h"

#include "HttpApi/JsonField.h"
#include "Shelly/ShellyPlugId.h"
#include "Shelly/ShellyPlugLimits.h"
#include "Shelly/ShellySmokeId.h"

#include <Arduino.h>

namespace wattcycle::panel {

P4ShellyUdp::P4ShellyUdp(shelly::IShellyPlugHub& plugs, shelly::IShellySmokeHub& smoke)
    : plugs_(plugs), smoke_(smoke) {}

bool P4ShellyUdp::begin() {
  if (started_) {
    return true;
  }
  started_ = udp_.begin(shelly::kShellyUdpPort);
  if (started_) {
    Serial.printf("shelly udp listen %u\n", static_cast<unsigned>(shelly::kShellyUdpPort));
  }
  return started_;
}

void P4ShellyUdp::loop(uint32_t nowMs, std::time_t epochUtc) {
  if (!started_) {
    return;
  }
  plugs_.tick(nowMs, epochUtc);
  smoke_.tick(nowMs);
  int packetSize = udp_.parsePacket();
  while (packetSize > 0) {
    const int read = udp_.read(buffer_, shelly::kShellyUdpBuffer);
    if (read > 0) {
      buffer_[read] = '\0';
      char src[shelly::kShellyIdCapacity] = {};
      if (web::extractJsonStringField(buffer_, "src", src, sizeof(src))) {
        if (shelly::shellySmokeIdValid(src)) {
          smoke_.ingest(buffer_, nowMs);
        } else if (shelly::shellyPlugIdValid(src)) {
          plugs_.ingest(buffer_, nowMs, epochUtc);
        }
      }
    }
    packetSize = udp_.parsePacket();
  }
}

}  // namespace wattcycle::panel
