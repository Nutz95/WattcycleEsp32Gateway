#include "Telemetry/InMemoryTelemetryStore.h"

#include "Util/SafeCopy.h"

namespace xt369p::telemetry {

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

void InMemoryTelemetryStore::updateMeter(const meter::WattmeterTelemetry& telemetry) {
  lock();
  meter_ = telemetry;
  status_.telemetryFresh = telemetry.valid;
  unlock();
}

void InMemoryTelemetryStore::setEspNowState(bool ready, uint8_t channel, const char* peerMac,
                                           const char* error) {
  lock();
  status_.espNowReady = ready;
  status_.espNowChannel = channel;
  util::copyCString(status_.espNowPeerMac, sizeof(status_.espNowPeerMac),
                    peerMac ? peerMac : "");
  if (error != nullptr && error[0] != '\0') {
    util::copyCString(status_.lastError, sizeof(status_.lastError), error);
  }
  unlock();
}

void InMemoryTelemetryStore::setSppState(bool connected, const char* target, const char* error) {
  lock();
  status_.sppConnected = connected;
  util::copyCString(status_.sppTarget, sizeof(status_.sppTarget), target ? target : "");
  util::copyCString(status_.lastError, sizeof(status_.lastError), error ? error : "");
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

bool InMemoryTelemetryStore::enqueueMeterCommand(meter::MeterCommand command) {
  if (command == meter::MeterCommand::None) {
    return false;
  }
  lock();
  pendingCommand_ = command;
  unlock();
  return true;
}

bool InMemoryTelemetryStore::dequeueMeterCommand(meter::MeterCommand& command) {
  lock();
  command = pendingCommand_;
  pendingCommand_ = meter::MeterCommand::None;
  unlock();
  return command != meter::MeterCommand::None;
}

meter::WattmeterTelemetry InMemoryTelemetryStore::meter() const {
  lock();
  const auto copy = meter_;
  unlock();
  return copy;
}

GatewayStatus InMemoryTelemetryStore::status() const {
  lock();
  const auto copy = status_;
  unlock();
  return copy;
}

EspHealth InMemoryTelemetryStore::espHealth() const {
  lock();
  const auto copy = espHealth_;
  unlock();
  return copy;
}

}  // namespace xt369p::telemetry
