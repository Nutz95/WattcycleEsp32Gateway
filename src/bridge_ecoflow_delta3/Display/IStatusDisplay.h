#pragma once

#include "EcoFlow/Ble/EcoFlowBleSnapshot.h"
#include "Esp/EspHealthSnapshot.h"

#include <cstdint>

namespace ecoflow::display {

/// Local UI for EcoFlow bridge boards (Null on headless DevKit).
class IStatusDisplay {
 public:
  virtual ~IStatusDisplay() = default;
  /// Init panel / backlight (no-op for Null).
  virtual void begin() = 0;
  /// Draw latest BLE + health snapshot.
  virtual void render(const ble::EcoFlowBleSnapshot& bleSnapshot,
                      const esp_sys::EspHealthSnapshot& health) = 0;
  /// Advance to the next UI page (wraps).
  virtual void nextPage() = 0;
  /// Go back one UI page (wraps).
  virtual void previousPage() = 0;
  /// Active page index (0-based).
  virtual uint8_t pageIndex() const = 0;
  /// Enable or disable panel output / backlight.
  virtual void setBacklight(bool on) = 0;
  /// True when the panel is considered on.
  virtual bool isBacklightOn() const = 0;
};

}  // namespace ecoflow::display
