#pragma once

#include "Display/IStatusDisplay.h"

namespace ecoflow::display {

/// Headless DevKit: no TFT.
class NullStatusDisplay : public IStatusDisplay {
 public:
  /// No-op panel init.
  void begin() override {}
  /// No-op render.
  void render(const ble::EcoFlowBleSnapshot&, const esp_sys::EspHealthSnapshot&,
              const DongleBridgeLink&) override {}
  /// No-op page advance.
  void nextPage() override {}
  /// No-op page back.
  void previousPage() override {}
  /// Always page 0.
  uint8_t pageIndex() const override { return 0; }
  /// No-op backlight.
  void setBacklight(bool) override {}
  /// Always off on headless.
  bool isBacklightOn() const override { return false; }
};

}  // namespace ecoflow::display
