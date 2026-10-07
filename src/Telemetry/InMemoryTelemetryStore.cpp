#include "Telemetry/InMemoryTelemetryStore.h"

#include <cstring>

namespace wattcycle::telemetry {

InMemoryTelemetryStore::InMemoryTelemetryStore() {
#ifndef UNIT_TEST
  mutex_ = xSemaphoreCreateMutex();
#endif
}

InMemoryTelemetryStore::~InMemoryTelemetryStore() {
#ifndef UNIT_TEST
  if (mutex_ != nullptr) {
    vSemaphoreDelete(mutex_);
    mutex_ = nullptr;
  }
#endif
}

void InMemoryTelemetryStore::lock() const {
#ifndef UNIT_TEST
  if (mutex_ != nullptr) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
  }
#endif
}

void InMemoryTelemetryStore::unlock() const {
#ifndef UNIT_TEST
  if (mutex_ != nullptr) {
    xSemaphoreGive(mutex_);
  }
#endif
}

void InMemoryTelemetryStore::updateBattery(const bms::BatteryTelemetry& telemetry) {
  lock();
  battery_ = telemetry;
  unlock();
}

void InMemoryTelemetryStore::updateProduct(const bms::ProductInfo& product) {
  lock();
  product_ = product;
  unlock();
}

void InMemoryTelemetryStore::updateWarnings(const bms::WarningFlags& warnings) {
  lock();
  warnings_ = warnings;
  unlock();
}

void InMemoryTelemetryStore::setWifiState(bool connected, const char* ipAddress) {
  lock();
  status_.wifiConnected = connected;
  if (ipAddress == nullptr) {
    status_.wifiIp[0] = '\0';
  } else {
    std::strncpy(status_.wifiIp, ipAddress, sizeof(status_.wifiIp) - 1);
    status_.wifiIp[sizeof(status_.wifiIp) - 1] = '\0';
  }
  unlock();
}

void InMemoryTelemetryStore::setBleState(bool connected, const char* address,
                                         const char* error) {
  lock();
  status_.bleConnected = connected;
  if (address != nullptr) {
    std::strncpy(status_.bleAddress, address, sizeof(status_.bleAddress) - 1);
    status_.bleAddress[sizeof(status_.bleAddress) - 1] = '\0';
  }
  if (error != nullptr) {
    std::strncpy(status_.lastError, error, sizeof(status_.lastError) - 1);
    status_.lastError[sizeof(status_.lastError) - 1] = '\0';
  }
  unlock();
}

void InMemoryTelemetryStore::setTelemetryFresh(bool fresh) {
  lock();
  status_.telemetryFresh = fresh;
  unlock();
}

bms::BatteryTelemetry InMemoryTelemetryStore::battery() const {
  lock();
  const bms::BatteryTelemetry copy = battery_;
  unlock();
  return copy;
}

bms::ProductInfo InMemoryTelemetryStore::product() const {
  lock();
  const bms::ProductInfo copy = product_;
  unlock();
  return copy;
}

bms::WarningFlags InMemoryTelemetryStore::warnings() const {
  lock();
  const bms::WarningFlags copy = warnings_;
  unlock();
  return copy;
}

GatewayStatus InMemoryTelemetryStore::status() const {
  lock();
  const GatewayStatus copy = status_;
  unlock();
  return copy;
}

}  // namespace wattcycle::telemetry
