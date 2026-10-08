#include "Meter/Protocol/AtorchCommands.h"

#include <cstring>

namespace xt369p::meter {

bool AtorchCommands::encode(MeterCommand command, uint8_t out[kFrameLen]) {
  if (out == nullptr || command == MeterCommand::None) {
    return false;
  }
  out[0] = 0xFF;
  out[1] = 0x55;
  out[2] = 0x11;
  out[3] = kDeviceTypeDc;
  out[4] = static_cast<uint8_t>(command);
  out[5] = 0;
  out[6] = 0;
  out[7] = 0;
  out[8] = 0;
  uint8_t sum = 0;
  for (size_t i = 2; i < 9; ++i) {
    sum = static_cast<uint8_t>(sum + out[i]);
  }
  out[9] = static_cast<uint8_t>(sum ^ 0x44);
  return true;
}

const char* AtorchCommands::toWireName(MeterCommand command) {
  switch (command) {
    case MeterCommand::ResetWh:
      return "reset_wh";
    case MeterCommand::ResetAh:
      return "reset_ah";
    case MeterCommand::ResetDuration:
      return "reset_duration";
    case MeterCommand::ResetAll:
      return "reset_all";
    case MeterCommand::None:
      return "none";
  }
  return "none";
}

MeterCommand AtorchCommands::fromWireName(const char* name) {
  if (name == nullptr) {
    return MeterCommand::None;
  }
  if (std::strcmp(name, "reset_wh") == 0) {
    return MeterCommand::ResetWh;
  }
  if (std::strcmp(name, "reset_ah") == 0) {
    return MeterCommand::ResetAh;
  }
  if (std::strcmp(name, "reset_duration") == 0) {
    return MeterCommand::ResetDuration;
  }
  if (std::strcmp(name, "reset_all") == 0) {
    return MeterCommand::ResetAll;
  }
  return MeterCommand::None;
}

}  // namespace xt369p::meter
