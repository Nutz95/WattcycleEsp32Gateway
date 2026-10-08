#include "Bms/Protocol/FrameBuilder.h"

#include "Bms/Protocol/ModbusCrc16.h"

namespace wattcycle::bms {

size_t FrameBuilder::buildReadFrame(uint16_t dataPointAddress, uint8_t frameHead,
                                    uint8_t* outBuffer, size_t outCapacity) {
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

size_t FrameBuilder::buildAnalogQuantityRead(uint8_t frameHead, uint8_t* outBuffer,
                                             size_t outCapacity) {
  return buildReadFrame(kDpAnalogQuantity, frameHead, outBuffer, outCapacity);
}

size_t FrameBuilder::buildWarningInfoRead(uint8_t frameHead, uint8_t* outBuffer,
                                          size_t outCapacity) {
  return buildReadFrame(kDpWarningInfo, frameHead, outBuffer, outCapacity);
}

size_t FrameBuilder::buildProductInfoRead(uint8_t frameHead, uint8_t* outBuffer,
                                          size_t outCapacity) {
  return buildReadFrame(kDpProductInfo, frameHead, outBuffer, outCapacity);
}

}  // namespace wattcycle::bms
