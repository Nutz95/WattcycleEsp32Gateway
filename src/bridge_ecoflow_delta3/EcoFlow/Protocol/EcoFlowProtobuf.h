#pragma once

#include "EcoFlow/Protocol/ProtoFieldView.h"

#include <cstddef>
#include <cstdint>

namespace ecoflow::protocol {

/// Minimal protobuf walker (varint / fixed32 / fixed64 / length-delimited).
class EcoFlowProtobuf {
 public:
  using FieldCallback = bool (*)(const ProtoFieldView& field, void* user);

  /// Walks all top-level fields. Callback return false to stop early.
  static bool walk(const uint8_t* data, size_t length, FieldCallback callback, void* user);

  /// Decode one varint at pos; advances pos on success.
  static bool readVarint(const uint8_t* data, size_t length, size_t& pos, uint64_t& out);
};

}  // namespace ecoflow::protocol
