#pragma once

#include "Display/IStatusDisplay.h"
#include "boards/dongle_s3/DonglePanelShell.h"

#include <cstdint>

namespace ecoflow::display {

/// 160x80 EcoFlow UI: overview, inputs, outputs, pack, meter, link, ESP.
class DongleStatusDisplay : public IStatusDisplay {
 public:
  /// Init ST7735 + framebuffers.
  void begin() override;
  /// Draw the active page from BLE, ESP-NOW, and health snapshots.
  void render(const ble::EcoFlowBleSnapshot& bleSnapshot,
              const esp_sys::EspHealthSnapshot& health, const DongleBridgeLink& link) override;
  /// Advance page (wraps).
  void nextPage() override;
  /// Go back one page (wraps).
  void previousPage() override;
  /// Current page index 0..6.
  uint8_t pageIndex() const override;
  /// Toggle panel backlight GPIO (or DISPOFF when hard-wired).
  void setBacklight(bool on) override;
  /// Whether backlight / panel output is currently on.
  bool isBacklightOn() const override;

 private:
  static constexpr uint8_t kPageCount = 7;

  void drawOverview(const ble::EcoFlowBleSnapshot& bleSnapshot, const DongleBridgeLink& link);
  void drawInputs(const ble::EcoFlowBleSnapshot& bleSnapshot);
  void drawOutputs(const ble::EcoFlowBleSnapshot& bleSnapshot);
  void drawPack(const ble::EcoFlowBleSnapshot& bleSnapshot);
  void drawMeter(const ble::EcoFlowBleSnapshot& bleSnapshot);
  void drawLink(const ble::EcoFlowBleSnapshot& bleSnapshot, const DongleBridgeLink& link);
  void drawEsp(const esp_sys::EspHealthSnapshot& health);

  boards::dongle_s3::DonglePanelShell panel_;
  uint8_t pageIndex_ = 0;
};

}  // namespace ecoflow::display
