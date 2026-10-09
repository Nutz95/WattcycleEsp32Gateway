#pragma once

#include <cstdint>

namespace ecoflow::protocol {

enum class ProtoWireType : uint8_t {
  Varint = 0,
  Fixed64 = 1,
  LengthDelimited = 2,
  Fixed32 = 5,
};

}  // namespace ecoflow::protocol
