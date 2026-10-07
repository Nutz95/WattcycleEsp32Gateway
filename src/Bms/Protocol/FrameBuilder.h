#pragma once

#include <cstddef>
#include <cstdint>

#include "Bms/Protocol/ProtocolConstants.h"

namespace wattcycle::bms {

/// Builds Wattcycle read-request frames (old protocol, no infoData).
class FrameBuilder {
 public:
  /// Writes a complete read frame into `outBuffer`. Returns bytes written, or 0 on failure.
  static size_t buildReadFrame(uint16_t dataPointAddress, uint8_t frameHead,
                               uint8_t* outBuffer, size_t outCapacity);

  static size_t buildAnalogQuantityRead(uint8_t frameHead, uint8_t* outBuffer,
                                        size_t outCapacity);

  static size_t buildWarningInfoRead(uint8_t frameHead, uint8_t* outBuffer,
                                     size_t outCapacity);

  static size_t buildProductInfoRead(uint8_t frameHead, uint8_t* outBuffer,
                                     size_t outCapacity);
};

}  // namespace wattcycle::bms
