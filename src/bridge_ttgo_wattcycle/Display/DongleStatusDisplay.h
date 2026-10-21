#pragma once

#include "Display/IStatusDisplay.h"
#include "boards/dongle_s3/DonglePanelShell.h"

#include <cstdint>

namespace wattcycle::display {

/// 160x80 BMS UI with the same pages as the TTGO bridge (SoC, cells, temps, alerts, link, ESP).
class DongleStatusDisplay : public IStatusDisplay {
 public:
  /// Init ST7735 + framebuffers.
  void begin() override;
  /// Draw the active page from the telemetry store.
  void render(const telemetry::ITelemetryStore& store) override;
  /// Advance page (wraps).
  void nextPage() override;
  /// Go back one page (wraps).
  void previousPage() override;
  /// Current page index 0..5.
  uint8_t pageIndex() const override;
  /// Toggle panel backlight GPIO (or DISPOFF when hard-wired).
  void setBacklight(bool on) override;
  /// Whether backlight / panel output is currently on.
  bool isBacklightOn() const override;

 private:
  static constexpr uint8_t kPageCount = 6;

  void drawOverview(const telemetry::ITelemetryStore& store);
  void drawCells(const telemetry::ITelemetryStore& store);
  void drawTemps(const telemetry::ITelemetryStore& store);
  void drawAlerts(const telemetry::ITelemetryStore& store);
  void drawLink(const telemetry::ITelemetryStore& store);
  void drawEsp(const telemetry::ITelemetryStore& store);

  boards::dongle_s3::DonglePanelShell panel_;
  uint8_t pageIndex_ = 0;
};

}  // namespace wattcycle::display
