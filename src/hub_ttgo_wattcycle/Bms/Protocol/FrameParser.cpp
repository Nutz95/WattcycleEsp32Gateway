#include "Bms/Protocol/FrameParser.h"

#include "Bms/Protocol/ModbusCrc16.h"
#include "Bms/Protocol/ProtocolConstants.h"

namespace wattcycle::bms {

int FrameParser::expectedResponseLength(const uint8_t* firstPacket, size_t length) {
  if (firstPacket == nullptr || length < 8) {
    return -1;
  }
  const uint16_t dataLength =
      static_cast<uint16_t>((firstPacket[6] << 8) | firstPacket[7]);
  return static_cast<int>(dataLength) + 11;
}

ParsedFrame FrameParser::parse(const uint8_t* frame, size_t length) {
  ParsedFrame result;
  if (frame == nullptr || length < kMinFrameSize) {
    return result;
  }
  if (frame[0] != kFrameHead && frame[0] != kFrameHeadAlt) {
    return result;
  }
  if (frame[length - 1] != kFrameTail) {
    return result;
  }
  if (frame[3] == kFuncError) {
    return result;
  }
  if (!ModbusCrc16::verifyFrame(frame, length)) {
    return result;
  }

  result.version = frame[1];
  result.address = frame[2];
  result.functionCode = frame[3];
  result.startAddress = static_cast<uint16_t>((frame[4] << 8) | frame[5]);
  result.dataLength = static_cast<uint16_t>((frame[6] << 8) | frame[7]);
  if (static_cast<size_t>(result.dataLength) + kMinFrameSize > length) {
    return ParsedFrame{};
  }
  result.data = &frame[8];
  result.valid = true;
  return result;
}

}  // namespace wattcycle::bms
