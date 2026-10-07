#pragma once

#include "Telemetry/ITelemetryStore.h"

namespace wattcycle::display {

class IStatusDisplay {
 public:
  virtual ~IStatusDisplay() = default;

  virtual void begin() = 0;
  virtual void render(const telemetry::ITelemetryStore& store) = 0;
};

}  // namespace wattcycle::display
