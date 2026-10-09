#include "Esp/EspHealthSampler.h"

#include "Config/TimingConstants.h"

#include <Arduino.h>
#include <esp_freertos_hooks.h>

namespace ecoflow::esp_sys {
namespace {

volatile uint32_t gIdleTicks0 = 0;
volatile uint32_t gIdleTicks1 = 0;

bool onIdleCore0() {
  gIdleTicks0++;
  return true;
}

bool onIdleCore1() {
  gIdleTicks1++;
  return true;
}

uint8_t loadFromIdle(uint32_t idleDelta, uint32_t elapsedMs, uint32_t& maxIdlePerSec) {
  if (elapsedMs == 0) {
    return 0;
  }
  const uint32_t idlePerSec = (idleDelta * 1000u) / elapsedMs;
  if (idlePerSec > maxIdlePerSec) {
    maxIdlePerSec = idlePerSec;
  }
  if (maxIdlePerSec == 0) {
    return 0;
  }
  uint32_t idlePercent = (idlePerSec * 100u) / maxIdlePerSec;
  if (idlePercent > 100u) {
    idlePercent = 100u;
  }
  return static_cast<uint8_t>(100u - idlePercent);
}

}  // namespace

void EspHealthSampler::begin() {
  esp_register_freertos_idle_hook_for_cpu(onIdleCore0, 0);
  esp_register_freertos_idle_hook_for_cpu(onIdleCore1, 1);
  lastSampleMs_ = millis();
  prevIdle0_ = gIdleTicks0;
  prevIdle1_ = gIdleTicks1;
}

bool EspHealthSampler::sample() {
  const uint32_t nowMs = millis();
  const uint32_t elapsedMs = nowMs - lastSampleMs_;
  if (elapsedMs < config::TimingConstants::kEspHealthSampleMs) {
    return false;
  }

  const uint32_t idle0 = gIdleTicks0;
  const uint32_t idle1 = gIdleTicks1;
  health_.cpuCore0Percent = loadFromIdle(idle0 - prevIdle0_, elapsedMs, maxIdle0PerSec_);
  health_.cpuCore1Percent = loadFromIdle(idle1 - prevIdle1_, elapsedMs, maxIdle1PerSec_);
  prevIdle0_ = idle0;
  prevIdle1_ = idle1;
  lastSampleMs_ = nowMs;

  health_.chipTemperatureC = temperatureRead();
  health_.freeHeapBytes = ESP.getFreeHeap();
  health_.uptimeSeconds = nowMs / 1000u;
  return true;
}

EspHealthSnapshot EspHealthSampler::snapshot() const {
  return health_;
}

}  // namespace ecoflow::esp_sys
