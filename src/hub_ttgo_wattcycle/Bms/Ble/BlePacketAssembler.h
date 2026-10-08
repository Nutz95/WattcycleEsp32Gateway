#pragma once

#include <cstddef>
#include <cstdint>

#include "Bms/Protocol/ProtocolConstants.h"

namespace wattcycle::bms {

/// Reassembles fragmented BLE notifications into a complete frame.
class BlePacketAssembler {
 public:
  void reset();
  void append(const uint8_t* chunk, size_t length);
  bool isComplete() const;
  const uint8_t* data() const;
  size_t size() const;

 private:
  uint8_t buffer_[kMaxFrameBytes] = {};
  size_t length_ = 0;
  int expectedLength_ = -1;
};

}  // namespace wattcycle::bms
