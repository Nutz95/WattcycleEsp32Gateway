#pragma once

#include "Display/IStatusDisplay.h"
#include "boards/dongle_s3/DonglePanelShell.h"

#include <cstdint>

namespace ecoflow::display {

/// Compact 160x80 EcoFlow UI (4 pages) on Pocket dongle via shared panel shell.
class DongleStatusDisplay : public IStatusDisplay {
 public:
  /// Init ST7735 + framebuffers.
  void begin() override;
  /// Draw active page from BLE + health snapshots.
  void render(const ble::EcoFlowBleSnapshot& bleSnapshot,
              const esp_sys::EspHealthSnapshot& health) override;
  /// Advance page (wraps).
  void nextPage() override;
  /// Go back one page (wraps).
  void previousPage() override;
  /// Current page index 0..3.
  uint8_t pageIndex() const override;
  /// Toggle panel backlight GPIO (or DISPOFF when hard-wired).
  void setBacklight(bool on) override;
  /// Whether backlight / panel output is currently on.
  bool isBacklightOn() const override;

 private:
  static constexpr uint8_t kPageCount = 4;

  void drawOverview(const ble::EcoFlowBleSnapshot& bleSnapshot);
  void drawPorts(const ble::EcoFlowBleSnapshot& bleSnapshot);
  void drawLink(const ble::EcoFlowBleSnapshot& bleSnapshot);
  void drawEsp(const esp_sys::EspHealthSnapshot& health);

  boards::dongle_s3::DonglePanelShell panel_;
  uint8_t pageIndex_ = 0;
};

}  // namespace ecoflow::display
