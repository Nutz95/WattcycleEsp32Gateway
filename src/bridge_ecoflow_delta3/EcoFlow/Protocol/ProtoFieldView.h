#pragma once

#include "EcoFlow/Protocol/ProtoWireType.h"

#include <cstdint>

namespace ecoflow::protocol {

struct ProtoFieldView {
  uint32_t number = 0;
  ProtoWireType wireType = ProtoWireType::Varint;
  uint64_t varint = 0;
  float float32 = 0.0f;
  const uint8_t* bytes = nullptr;
  uint32_t bytesLength = 0;
};

}  // namespace ecoflow::protocol
