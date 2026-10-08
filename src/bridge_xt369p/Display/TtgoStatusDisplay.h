#pragma once

#include "Display/IStatusDisplay.h"

#ifndef UNIT_TEST
#include <TFT_eSPI.h>
#endif

namespace xt369p::display {

class TtgoStatusDisplay : public IStatusDisplay {
 public:
  void begin() override;
  void render(const telemetry::ITelemetryStore& store) override;
  void nextPage() override;
  void previousPage() override;
  uint8_t pageIndex() const override;
  void setBacklight(bool on) override;
  bool isBacklightOn() const override;

 private:
  static constexpr uint8_t kPageCount = 3;

  void drawChrome(const char* title);
  void drawOverview(const telemetry::ITelemetryStore& store);
  void drawGateway(const telemetry::ITelemetryStore& store);
  void drawEsp(const telemetry::ITelemetryStore& store);
  void drawMetric(int x, int y, const char* label, const char* value);

  bool ready_ = false;
  bool backlightOn_ = true;
  uint8_t pageIndex_ = 0;
#ifndef UNIT_TEST
  TFT_eSPI tft_{};
  TFT_eSprite sprite_{&tft_};
#endif
};

}  // namespace xt369p::display
