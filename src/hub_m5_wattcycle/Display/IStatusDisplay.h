#pragma once

#include <cstdint>

#include "Auth/AuthTypes.h"
#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::display {

class IStatusDisplay {
 public:
  virtual ~IStatusDisplay() = default;

  virtual void begin() = 0;
  virtual void render(const telemetry::ITelemetryStore& store) = 0;
  virtual void renderAuthPrompt(const auth::AuthPrompt& prompt) = 0;
  virtual void nextPage() = 0;
  virtual void previousPage() = 0;
  virtual uint8_t pageIndex() const = 0;
  virtual void setBacklight(bool on) = 0;
  virtual bool isBacklightOn() const = 0;
};

}  // namespace wattcycle::display
