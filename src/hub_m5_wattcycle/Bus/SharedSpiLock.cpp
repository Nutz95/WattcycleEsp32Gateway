#include "Bus/SharedSpiLock.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace wattcycle::bus {
namespace {
SemaphoreHandle_t gSpiMutex = nullptr;
}  // namespace

void SharedSpiLock::begin() {
  if (gSpiMutex == nullptr) {
    gSpiMutex = xSemaphoreCreateMutex();
  }
}

void SharedSpiLock::lock() {
  if (gSpiMutex != nullptr) {
    xSemaphoreTake(gSpiMutex, portMAX_DELAY);
  }
}

void SharedSpiLock::unlock() {
  if (gSpiMutex != nullptr) {
    xSemaphoreGive(gSpiMutex);
  }
}

}  // namespace wattcycle::bus
