#pragma once

#include "Display/IStatusDisplay.h"

namespace wattcycle::display {

/// Renders a compact status screen on the TTGO T-Display ST7789V.
class TtgoStatusDisplay : public IStatusDisplay {
 public:
  void begin() override;
  void render(const telemetry::ITelemetryStore& store) override;

 private:
  bool ready_ = false;
};

}  // namespace wattcycle::display
