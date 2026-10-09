#pragma once

#include <cstddef>
#include <cstdint>

namespace ecoflow::ble {

using EcoFlowNotifyFn = void (*)(void* owner, const uint8_t* data, size_t length);

/// NimBLE scan/connect/GATT helpers for the EcoFlow bridge (no auth logic).
class EcoFlowBleGatt {
 public:
  EcoFlowBleGatt();
  ~EcoFlowBleGatt();

  /// Scan for device, connect, and learn BLE address type when found.
  bool connectFromScan(const char* bleAddress, uint8_t& outAddressType, bool& outHaveType);
  /// Connect using a known address and optional public/random type.
  bool connectByAddress(const char* bleAddress, bool haveType, uint8_t addressType);
  /// Discover EcoFlow write/notify characteristics and subscribe.
  bool resolveCharacteristics(EcoFlowNotifyFn notifyFn, void* notifyOwner);
  /// Write bytes to the EcoFlow TX characteristic.
  bool write(const uint8_t* data, size_t length);
  /// Tear down the NimBLE client link.
  void disconnect();
  /// True when connected and characteristics are ready.
  bool isLinkUp() const;
  /// Invoke registered notify callback (for tests / trampolines).
  void dispatchNotify(const uint8_t* data, size_t length) const;

 private:
  void configureClientLink() const;

  void* clientHandle_ = nullptr;
  void* writeCharacteristic_ = nullptr;
  void* notifyCharacteristic_ = nullptr;
  EcoFlowNotifyFn notifyFn_ = nullptr;
  void* notifyOwner_ = nullptr;
};

}  // namespace ecoflow::ble
