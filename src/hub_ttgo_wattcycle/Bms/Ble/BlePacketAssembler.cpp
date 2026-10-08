#include "Bms/Ble/BlePacketAssembler.h"

#include "Bms/Protocol/FrameParser.h"

#include <cstring>

namespace wattcycle::bms {

void BlePacketAssembler::reset() {
  length_ = 0;
  expectedLength_ = -1;
}

void BlePacketAssembler::append(const uint8_t* chunk, size_t length) {
  if (chunk == nullptr || length == 0) {
    return;
  }
  if (length_ + length > kMaxFrameBytes) {
    reset();
    return;
  }
  std::memcpy(buffer_ + length_, chunk, length);
  length_ += length;

  if (expectedLength_ < 0 && length_ >= 8) {
    expectedLength_ = FrameParser::expectedResponseLength(buffer_, length_);
  }
}

bool BlePacketAssembler::isComplete() const {
  return expectedLength_ > 0 && static_cast<int>(length_) >= expectedLength_;
}

const uint8_t* BlePacketAssembler::data() const {
  return buffer_;
}

size_t BlePacketAssembler::size() const {
  return length_;
}

}  // namespace wattcycle::bms
