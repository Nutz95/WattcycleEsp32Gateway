#pragma once

#include "Telemetry/ITelemetryStore.h"

#ifndef UNIT_TEST
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace xt369p::telemetry {

class InMemoryTelemetryStore : public ITelemetryStore {
 public:
  InMemoryTelemetryStore();
  ~InMemoryTelemetryStore() override;

  void updateMeter(const meter::WattmeterTelemetry& telemetry) override;
  void setEspNowState(bool ready, uint8_t channel, const char* peerMac,
                      const char* error) override;
  void setSppState(bool connected, const char* target, const char* error) override;
  void setTelemetryFresh(bool fresh) override;
  void updateEspHealth(const EspHealth& health) override;

  bool enqueueMeterCommand(meter::MeterCommand command) override;
  bool dequeueMeterCommand(meter::MeterCommand& command) override;

  meter::WattmeterTelemetry meter() const override;
  GatewayStatus status() const override;
  EspHealth espHealth() const override;

 private:
  void lock() const;
  void unlock() const;

  meter::WattmeterTelemetry meter_{};
  GatewayStatus status_{};
  EspHealth espHealth_{};
  meter::MeterCommand pendingCommand_ = meter::MeterCommand::None;
#ifndef UNIT_TEST
  SemaphoreHandle_t mutex_ = nullptr;
#endif
};

}  // namespace xt369p::telemetry
