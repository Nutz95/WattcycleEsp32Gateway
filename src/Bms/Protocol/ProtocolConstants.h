#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::bms {

/// BLE GATT UUIDs for XDZN / Wattcycle BMS (service 0xFFF0).
constexpr const char* kServiceUuid = "0000fff0-0000-1000-8000-00805f9b34fb";
constexpr const char* kWriteUuid = "0000fff2-0000-1000-8000-00805f9b34fb";
constexpr const char* kNotifyUuid = "0000fff1-0000-1000-8000-00805f9b34fb";
constexpr const char* kAuthUuid = "0000fffa-0000-1000-8000-00805f9b34fb";

constexpr const char* kAuthKey = "HiLink";

constexpr uint8_t kFrameHead = 0x7E;
constexpr uint8_t kFrameHeadAlt = 0x1E;
constexpr uint8_t kFrameTail = 0x0D;
constexpr uint8_t kFuncRead = 0x03;
constexpr uint8_t kFuncError = 0x86;
constexpr uint8_t kDeviceAddr = 0x01;
constexpr size_t kMinFrameSize = 11;

constexpr uint16_t kDpAnalogQuantity = 140;
constexpr uint16_t kDpWarningInfo = 141;
constexpr uint16_t kDpProductInfo = 146;

constexpr size_t kMaxFrameBytes = 512;
constexpr size_t kMaxCellCount = 32;

}  // namespace wattcycle::bms
