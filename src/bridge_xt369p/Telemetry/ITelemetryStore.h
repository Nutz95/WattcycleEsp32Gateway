#pragma once

#include "Meter/Models/WattmeterTelemetry.h"
#include "Meter/Protocol/AtorchCommands.h"
#include "Telemetry/EspHealth.h"
#include "Telemetry/GatewayStatus.h"

namespace xt369p::telemetry {

class ITelemetryStore {
 public:
  virtual ~ITelemetryStore() = default;

  virtual void updateMeter(const meter::WattmeterTelemetry& telemetry) = 0;
  virtual void setEspNowState(bool ready, uint8_t channel, const char* peerMac,
                              const char* error) = 0;
  virtual void setSppState(bool connected, const char* target, const char* error) = 0;
  virtual void setTelemetryFresh(bool fresh) = 0;
  virtual void updateEspHealth(const EspHealth& health) = 0;

  virtual bool enqueueMeterCommand(meter::MeterCommand command) = 0;
  virtual bool dequeueMeterCommand(meter::MeterCommand& command) = 0;

  virtual meter::WattmeterTelemetry meter() const = 0;
  virtual GatewayStatus status() const = 0;
  virtual EspHealth espHealth() const = 0;
};

}  // namespace xt369p::telemetry
