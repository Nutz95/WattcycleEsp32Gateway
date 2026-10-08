#pragma once

#include "Display/IStatusDisplay.h"

namespace wattcycle::display {

/// Compact status pages on M5Stack Basic (ILI9342 via M5Unified).
class M5StatusDisplay : public IStatusDisplay {
 public:
  void begin() override;
  void render(const telemetry::ITelemetryStore& store) override;
  void renderAuthPrompt(const auth::AuthPrompt& prompt) override;
  void nextPage() override;
  void previousPage() override;
  uint8_t pageIndex() const override;
  void setBacklight(bool on) override;
  bool isBacklightOn() const override;

 private:
  void drawOverview(const telemetry::ITelemetryStore& store);
  void drawGateway(const telemetry::ITelemetryStore& store);

  bool ready_ = false;
  bool backlightOn_ = true;
  uint8_t pageIndex_ = 0;
};

}  // namespace wattcycle::display
