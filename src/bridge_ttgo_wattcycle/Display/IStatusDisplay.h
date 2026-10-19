#pragma once

#include <cstdint>

#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::display {

/// Local status UI for BMS bridge boards (TTGO or Pocket-Dongle).
class IStatusDisplay {
 public:
  virtual ~IStatusDisplay() = default;

  /// Init panel / backlight.
  virtual void begin() = 0;
  /// Draw the active page from live telemetry.
  virtual void render(const telemetry::ITelemetryStore& store) = 0;
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

}  // namespace wattcycle::display
