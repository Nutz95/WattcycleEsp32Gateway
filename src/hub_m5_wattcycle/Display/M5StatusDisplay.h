#pragma once

#include "Display/IStatusDisplay.h"

#ifndef UNIT_TEST
#include <M5Unified.h>
#endif

namespace wattcycle::display {

/// Compact status pages on M5Stack Basic (ILI9342 via M5Unified).
/// Draws into an off-screen canvas then pushSprite — same anti-flicker
/// approach as the TTGO TFT_eSprite UI.
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
  void drawPack(const telemetry::ITelemetryStore& store);
  void drawSolar(const telemetry::ITelemetryStore& store);
  void drawGateway(const telemetry::ITelemetryStore& store);
  void drawEsp(const telemetry::ITelemetryStore& store);
  void drawTemps(const telemetry::ITelemetryStore& store);
  void drawButtonFooter(const char* centerLabel);
  void pushFrame();

  bool ready_ = false;
  bool backlightOn_ = true;
  uint8_t pageIndex_ = 0;
#ifndef UNIT_TEST
  M5Canvas canvas_{&M5.Display};
#endif
};

}  // namespace wattcycle::display
