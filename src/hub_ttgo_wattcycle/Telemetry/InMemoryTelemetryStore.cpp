#include "Telemetry/InMemoryTelemetryStore.h"

#include "Util/SafeCopy.h"

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

void InMemoryTelemetryStore::updateSolar(const SolarBridgeTelemetry& solar) {
  lock();
  solar_ = solar;
  unlock();
}

void InMemoryTelemetryStore::setWifiState(bool connected, const char* ipAddress) {
  lock();
  status_.wifiConnected = connected;
  wattcycle::util::copyCString(status_.wifiIp, sizeof(status_.wifiIp), ipAddress);
  unlock();
}

void InMemoryTelemetryStore::setWebPort(uint16_t port) {
  lock();
  status_.webPort = port;
  unlock();
}

void InMemoryTelemetryStore::setBleState(bool connected, const char* address,
                                         const char* error) {
  lock();
  status_.bleConnected = connected;
  if (address != nullptr) {
    wattcycle::util::copyCString(status_.bleAddress, sizeof(status_.bleAddress), address);
  }
  if (error != nullptr) {
    wattcycle::util::copyCString(status_.lastError, sizeof(status_.lastError), error);
  }
  unlock();
}

void InMemoryTelemetryStore::setTelemetryFresh(bool fresh) {
  lock();
  status_.telemetryFresh = fresh;
  unlock();
}

void InMemoryTelemetryStore::updateEspHealth(const EspHealth& health) {
  lock();
  espHealth_ = health;
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

SolarBridgeTelemetry InMemoryTelemetryStore::solar() const {
  lock();
  const SolarBridgeTelemetry copy = solar_;
  unlock();
  return copy;
}

GatewayStatus InMemoryTelemetryStore::status() const {
  lock();
  const GatewayStatus copy = status_;
  unlock();
  return copy;
}

EspHealth InMemoryTelemetryStore::espHealth() const {
  lock();
  const EspHealth copy = espHealth_;
  unlock();
  return copy;
}

}  // namespace wattcycle::telemetry
