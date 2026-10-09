#pragma once

#include "Telemetry/ITelemetryStore.h"

#ifndef UNIT_TEST
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace wattcycle::telemetry {

class InMemoryTelemetryStore : public ITelemetryStore {
 public:
  InMemoryTelemetryStore();
  ~InMemoryTelemetryStore() override;

  void updateBattery(const bms::BatteryTelemetry& telemetry) override;
  void updateProduct(const bms::ProductInfo& product) override;
  void updateWarnings(const bms::WarningFlags& warnings) override;
  void updateSolar(const SolarBridgeTelemetry& solar) override;
  void updateEcoFlow(const EcoFlowBridgeTelemetry& ecoflow) override;
  void setWifiState(bool connected, const char* ipAddress) override;
  void setNtpSynced(bool synced) override;
  void setLocalClock(const char* dateDdMmYyyy, const char* timeHhMm) override;
  void setWebPort(uint16_t port) override;
  void setBleState(bool connected, const char* address, const char* error) override;
  void setTelemetryFresh(bool fresh) override;
  void updateEspHealth(const EspHealth& health) override;
  void updateBmsBridgeHealth(const EspHealth& health, bool valid) override;

  bms::BatteryTelemetry battery() const override;
  bms::ProductInfo product() const override;
  bms::WarningFlags warnings() const override;
  SolarBridgeTelemetry solar() const override;
  EcoFlowBridgeTelemetry ecoflow() const override;
  GatewayStatus status() const override;
  EspHealth espHealth() const override;
  EspHealth bmsBridgeHealth() const override;
  bool bmsBridgeHealthValid() const override;

 private:
  void lock() const;
  void unlock() const;

  bms::BatteryTelemetry battery_{};
  bms::ProductInfo product_{};
  bms::WarningFlags warnings_{};
  SolarBridgeTelemetry solar_{};
  EcoFlowBridgeTelemetry ecoflow_{};
  GatewayStatus status_{};
  EspHealth espHealth_{};
  EspHealth bmsBridgeHealth_{};
  bool bmsBridgeHealthValid_ = false;
#ifndef UNIT_TEST
  SemaphoreHandle_t mutex_ = nullptr;
#endif
};

}  // namespace wattcycle::telemetry
