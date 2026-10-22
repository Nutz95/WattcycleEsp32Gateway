#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "Bms/Protocol/ModbusCrc16.h"
#include "Bms/Protocol/ProtocolConstants.h"

namespace wattcycle::bms {

/// Builds Wattcycle request frames (old protocol, no infoData).
class FrameBuilder {
 public:
  /// Writes a complete read frame into `outBuffer`. Returns bytes written, or 0 on failure.
  static size_t buildReadFrame(uint16_t dataPointAddress, uint8_t frameHead, uint8_t* outBuffer,
                               size_t outCapacity) {
    constexpr size_t kFrameLength = 11;
    if (outBuffer == nullptr || outCapacity < kFrameLength) {
      return 0;
    }

    outBuffer[0] = frameHead;
    outBuffer[1] = 0x00;
    outBuffer[2] = kDeviceAddr;
    outBuffer[3] = kFuncRead;
    outBuffer[4] = static_cast<uint8_t>((dataPointAddress >> 8) & 0xFF);
    outBuffer[5] = static_cast<uint8_t>(dataPointAddress & 0xFF);
    outBuffer[6] = 0x00;
    outBuffer[7] = 0x00;

    const uint16_t crc = ModbusCrc16::compute(outBuffer, 8);
    outBuffer[8] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    outBuffer[9] = static_cast<uint8_t>(crc & 0xFF);
    outBuffer[10] = kFrameTail;
    return kFrameLength;
  }

  /// Writes a FUNC 0x06 frame. `data` is the unpublished DP payload, copied as-is.
  static size_t buildWriteFrame(uint16_t dataPointAddress, const uint8_t* data, size_t dataLength,
                                uint8_t frameHead, uint8_t* outBuffer, size_t outCapacity) {
    if (outBuffer == nullptr || dataLength > 32 || (dataLength > 0 && data == nullptr)) {
      return 0;
    }
    const size_t frameLength = 11 + dataLength;
    if (outCapacity < frameLength) {
      return 0;
    }

    outBuffer[0] = frameHead;
    outBuffer[1] = 0x00;
    outBuffer[2] = kDeviceAddr;
    outBuffer[3] = kFuncWrite;
    outBuffer[4] = static_cast<uint8_t>((dataPointAddress >> 8) & 0xFF);
    outBuffer[5] = static_cast<uint8_t>(dataPointAddress & 0xFF);
    outBuffer[6] = static_cast<uint8_t>((dataLength >> 8) & 0xFF);
    outBuffer[7] = static_cast<uint8_t>(dataLength & 0xFF);
    if (dataLength > 0) {
      std::memcpy(outBuffer + 8, data, dataLength);
    }

    const size_t crcOffset = 8 + dataLength;
    const uint16_t crc = ModbusCrc16::compute(outBuffer, crcOffset);
    outBuffer[crcOffset] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    outBuffer[crcOffset + 1] = static_cast<uint8_t>(crc & 0xFF);
    outBuffer[crcOffset + 2] = kFrameTail;
    return frameLength;
  }

  static size_t buildAnalogQuantityRead(uint8_t frameHead, uint8_t* outBuffer, size_t outCapacity) {
    return buildReadFrame(kDpAnalogQuantity, frameHead, outBuffer, outCapacity);
  }

  static size_t buildWarningInfoRead(uint8_t frameHead, uint8_t* outBuffer, size_t outCapacity) {
    return buildReadFrame(kDpWarningInfo, frameHead, outBuffer, outCapacity);
  }

  static size_t buildProductInfoRead(uint8_t frameHead, uint8_t* outBuffer, size_t outCapacity) {
    return buildReadFrame(kDpProductInfo, frameHead, outBuffer, outCapacity);
  }
};

}  // namespace wattcycle::bms
