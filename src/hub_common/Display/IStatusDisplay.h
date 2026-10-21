#pragma once

#include <cstdint>

#include "Auth/AuthTypes.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::display {

/// Hub screen contract shared by the M5 pages and the P4 shell.
class IStatusDisplay {
 public:
  virtual ~IStatusDisplay() = default;

  /// Bring the panel up. Safe to call once from the composition root.
  virtual void begin() = 0;
  /// Paint the active page from the latest store snapshot.
  virtual void render(const telemetry::ITelemetryStore& store) = 0;
  /// Replace the page with a physical confirm/reset prompt.
  virtual void renderAuthPrompt(const auth::AuthPrompt& prompt) = 0;
  /// Advance to the next hub page.
  virtual void nextPage() = 0;
  /// Return to the previous hub page.
  virtual void previousPage() = 0;
  /// Zero-based index of the page currently shown.
  virtual uint8_t pageIndex() const = 0;
  /// Turn the backlight on or off without changing the page.
  virtual void setBacklight(bool on) = 0;
  /// True while the backlight is enabled.
  virtual bool isBacklightOn() const = 0;
};

}  // namespace wattcycle::display
