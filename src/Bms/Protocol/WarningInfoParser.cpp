#include "Bms/Protocol/WarningInfoParser.h"

namespace wattcycle::bms {

bool WarningInfoParser::parse(const uint8_t* data, size_t length, WarningFlags& out) {
  if (data == nullptr || length < 8) {
    return false;
  }

  WarningFlags flags;
  size_t offset = 0;
  const uint8_t cellCount = data[offset++];
  if (cellCount == 0 || offset + cellCount >= length) {
    return false;
  }
  offset += cellCount;

  if (offset >= length) {
    return false;
  }
  const uint8_t temperatureCount = data[offset++];
  if (temperatureCount < 2 || offset + temperatureCount >= length) {
    return false;
  }
  offset += temperatureCount;

  if (offset + 4 > length) {
    return false;
  }
  offset += 4;  // charge / voltage / discharge / mode

  if (offset + 3 > length) {
    return false;
  }
  flags.statusRegister1 = data[offset++];
  flags.statusRegister2 = data[offset++];
  offset += 1;  // status register 3 (unused in gateway summary)

  if (offset + 1 > length) {
    return false;
  }
  offset += 1;  // reserved

  if (offset >= length) {
    return false;
  }
  flags.statusRegister5 = data[offset++];

  if (offset + 2 > length) {
    return false;
  }
  offset += 2;  // reserved

  if (offset + 2 > length) {
    return false;
  }
  flags.warningRegister1 = data[offset++];
  flags.warningRegister2 = data[offset++];

  flags.hasActiveProtection =
      (flags.statusRegister1 != 0) || (flags.statusRegister2 != 0);
  flags.hasFault = (flags.statusRegister5 != 0);
  flags.valid = true;
  out = flags;
  return true;
}

}  // namespace wattcycle::bms
