#include "Bms/Protocol/ProductInfoParser.h"

#include <cstring>

namespace wattcycle::bms {
namespace {

void copyAsciiField(const uint8_t* source, char* destination, size_t fieldLength) {
  size_t copyLength = 0;
  while (copyLength < fieldLength && source[copyLength] != 0) {
    ++copyLength;
  }
  std::memcpy(destination, source, copyLength);
  destination[copyLength] = '\0';
  while (copyLength > 0 &&
         (destination[copyLength - 1] == ' ' || destination[copyLength - 1] == '\0')) {
    destination[--copyLength] = '\0';
  }
}

}  // namespace

bool ProductInfoParser::parse(const uint8_t* data, size_t length, ProductInfo& out) {
  if (data == nullptr || length != 60) {
    return false;
  }

  ProductInfo info;
  copyAsciiField(data, info.firmwareVersion, 20);
  copyAsciiField(data + 20, info.manufacturerName, 20);
  copyAsciiField(data + 40, info.serialNumber, 20);
  info.valid = true;
  out = info;
  return true;
}

}  // namespace wattcycle::bms
