#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::bms {

struct ParsedFrame {
  bool valid = false;
  uint8_t version = 0;
  uint8_t address = 0;
  uint8_t functionCode = 0;
  uint16_t startAddress = 0;
  uint16_t dataLength = 0;
  const uint8_t* data = nullptr;
};

/// Validates and slices a complete Wattcycle response frame.
class FrameParser {
 public:
  static int expectedResponseLength(const uint8_t* firstPacket, size_t length);
  static ParsedFrame parse(const uint8_t* frame, size_t length);
};

}  // namespace wattcycle::bms
