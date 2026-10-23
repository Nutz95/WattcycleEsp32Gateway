#include "P4ShellyUdp.h"

#include "Shelly/ShellyPlugLimits.h"

#include <Arduino.h>

namespace wattcycle::panel {

P4ShellyUdp::P4ShellyUdp(shelly::IShellyPlugHub& hub) : hub_(hub) {}

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
  hub_.tick(nowMs, epochUtc);
  int packetSize = udp_.parsePacket();
  while (packetSize > 0) {
    const int read = udp_.read(buffer_, shelly::kShellyUdpBuffer);
    if (read > 0) {
      buffer_[read] = '\0';
      hub_.ingest(buffer_, nowMs, epochUtc);
    }
    packetSize = udp_.parsePacket();
  }
}

}  // namespace wattcycle::panel
