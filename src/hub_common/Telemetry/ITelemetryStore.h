#pragma once

#include "Telemetry/ITelemetryReader.h"

namespace wattcycle::telemetry {

/// Read/write boundary for the latest BMS snapshot and gateway status.
class ITelemetryStore : public ITelemetryReader {
 public:
  ~ITelemetryStore() override = default;

  /// Replace the latest Wattcycle pack snapshot.
  virtual void updateBattery(const bms::BatteryTelemetry& telemetry) = 0;
  /// Replace BMS product identity text.
  virtual void updateProduct(const bms::ProductInfo& product) = 0;
  /// Replace active BMS warning flags.
  virtual void updateWarnings(const bms::WarningFlags& warnings) = 0;
  /// Replace the latest XT369P solar bridge snapshot.
  virtual void updateSolar(const SolarBridgeTelemetry& solar) = 0;
  /// Replace the latest EcoFlow bridge snapshot.
  virtual void updateEcoFlow(const EcoFlowBridgeTelemetry& ecoflow) = 0;
  /// Hub Wi-Fi association and STA IP text.
  virtual void setWifiState(bool connected, const char* ipAddress) = 0;
  /// True after NTP has set a usable UTC clock.
  virtual void setNtpSynced(bool synced) = 0;
  /// Local calendar strings shown on the panel.
  virtual void setLocalClock(const char* dateDdMmYyyy, const char* timeHhMm) = 0;
  /// HTTP listen port advertised to the browser.
  virtual void setWebPort(uint16_t port) = 0;
  /// BMS BLE connection, peer address, and last BLE error.
  virtual void setBleState(bool connected, const char* address, const char* error) = 0;
  /// True while ESP-NOW pack frames are arriving.
  virtual void setTelemetryFresh(bool fresh) = 0;
  /// Set BMS link flags together. Does not touch address or lastError.
  virtual void setBmsLink(bool telemetryFresh, bool bleConnected) = 0;
  /// Hub MCU health (CPU, heap, chip temp).
  virtual void updateEspHealth(const EspHealth& health) = 0;
  /// BMS bridge MCU health when `valid` is true.
  virtual void updateBmsBridgeHealth(const EspHealth& health, bool valid) = 0;
  /// Latest BMS frame was sealed with the ESP-NOW key.
  virtual void setBmsEspNowEncrypted(bool encrypted) = 0;
};

}  // namespace wattcycle::telemetry
