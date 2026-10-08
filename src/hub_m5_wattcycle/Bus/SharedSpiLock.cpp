#include "Bus/SharedSpiLock.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace wattcycle::bus {
namespace {
#ifndef UNIT_TEST
SemaphoreHandle_t gSpiMutex = nullptr;
#endif
}  // namespace

void SharedSpiLock::begin() {
#ifndef UNIT_TEST
  if (gSpiMutex == nullptr) {
    gSpiMutex = xSemaphoreCreateMutex();
  }
#endif
}

void SharedSpiLock::lock() {
#ifndef UNIT_TEST
  if (gSpiMutex != nullptr) {
    xSemaphoreTake(gSpiMutex, portMAX_DELAY);
  }
#endif
}

void SharedSpiLock::unlock() {
#ifndef UNIT_TEST
  if (gSpiMutex != nullptr) {
    xSemaphoreGive(gSpiMutex);
  }
#endif
}

}  // namespace wattcycle::bus
