#include "Telemetry/InMemoryTelemetryStore.h"

#include <cstring>

namespace wattcycle::telemetry {

void InMemoryTelemetryStore::updateBattery(const bms::BatteryTelemetry& telemetry) {
  battery_ = telemetry;
}

void InMemoryTelemetryStore::updateProduct(const bms::ProductInfo& product) {
  product_ = product;
}

void InMemoryTelemetryStore::updateWarnings(const bms::WarningFlags& warnings) {
  warnings_ = warnings;
}

void InMemoryTelemetryStore::setWifiState(bool connected, const char* ipAddress) {
  status_.wifiConnected = connected;
  if (ipAddress == nullptr) {
    status_.wifiIp[0] = '\0';
    return;
  }
  std::strncpy(status_.wifiIp, ipAddress, sizeof(status_.wifiIp) - 1);
  status_.wifiIp[sizeof(status_.wifiIp) - 1] = '\0';
}

void InMemoryTelemetryStore::setBleState(bool connected, const char* address,
                                         const char* error) {
  status_.bleConnected = connected;
  if (address != nullptr) {
    std::strncpy(status_.bleAddress, address, sizeof(status_.bleAddress) - 1);
    status_.bleAddress[sizeof(status_.bleAddress) - 1] = '\0';
  }
  if (error != nullptr) {
    std::strncpy(status_.lastError, error, sizeof(status_.lastError) - 1);
    status_.lastError[sizeof(status_.lastError) - 1] = '\0';
  }
}

void InMemoryTelemetryStore::setTelemetryFresh(bool fresh) {
  status_.telemetryFresh = fresh;
}

bms::BatteryTelemetry InMemoryTelemetryStore::battery() const {
  return battery_;
}

bms::ProductInfo InMemoryTelemetryStore::product() const {
  return product_;
}

bms::WarningFlags InMemoryTelemetryStore::warnings() const {
  return warnings_;
}

GatewayStatus InMemoryTelemetryStore::status() const {
  return status_;
}

}  // namespace wattcycle::telemetry
