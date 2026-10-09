#include "EcoFlow/Protocol/EcoFlowProtobuf.h"

#include <cstring>

namespace ecoflow::protocol {
namespace {

bool readFixed32Float(const uint8_t* data, size_t length, size_t& pos, float& out) {
  if (pos + 4 > length) {
    return false;
  }
  uint32_t bits = 0;
  std::memcpy(&bits, data + pos, 4);
  pos += 4;
  std::memcpy(&out, &bits, sizeof(out));
  return true;
}

bool skipFixed64(const uint8_t* data, size_t length, size_t& pos) {
  (void)data;
  if (pos + 8 > length) {
    return false;
  }
  pos += 8;
  return true;
}

bool readLengthBytes(const uint8_t* data, size_t length, size_t& pos, const uint8_t*& bytes,
                     uint32_t& bytesLength) {
  uint64_t size = 0;
  if (!EcoFlowProtobuf::readVarint(data, length, pos, size) || pos + size > length) {
    return false;
  }
  bytes = data + pos;
  bytesLength = static_cast<uint32_t>(size);
  pos += static_cast<size_t>(size);
  return true;
}

bool decodePayload(const uint8_t* data, size_t length, size_t& pos, ProtoFieldView& field) {
  switch (field.wireType) {
    case ProtoWireType::Varint:
      return EcoFlowProtobuf::readVarint(data, length, pos, field.varint);
    case ProtoWireType::Fixed32:
      return readFixed32Float(data, length, pos, field.float32);
    case ProtoWireType::Fixed64:
      return skipFixed64(data, length, pos);
    case ProtoWireType::LengthDelimited:
      return readLengthBytes(data, length, pos, field.bytes, field.bytesLength);
    default:
      return false;
  }
}

}  // namespace

bool EcoFlowProtobuf::readVarint(const uint8_t* data, size_t length, size_t& pos, uint64_t& out) {
  out = 0;
  uint32_t shift = 0;
  while (pos < length && shift < 64) {
    const uint8_t byte = data[pos++];
    out |= (static_cast<uint64_t>(byte & 0x7FU) << shift);
    if ((byte & 0x80U) == 0) {
      return true;
    }
    shift += 7;
  }
  return false;
}

bool EcoFlowProtobuf::walk(const uint8_t* data, size_t length, FieldCallback callback, void* user) {
  if (data == nullptr || callback == nullptr) {
    return false;
  }
  size_t pos = 0;
  while (pos < length) {
    uint64_t tag = 0;
    if (!readVarint(data, length, pos, tag)) {
      return false;
    }
    ProtoFieldView field{};
    field.number = static_cast<uint32_t>(tag >> 3);
    field.wireType = static_cast<ProtoWireType>(tag & 0x07U);
    if (!decodePayload(data, length, pos, field)) {
      return false;
    }
    if (!callback(field, user)) {
      return true;
    }
  }
  return true;
}

}  // namespace ecoflow::protocol
