#include "Meter/Protocol/AtorchFrameParser.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

#include <cstring>

namespace xt369p::meter {

void AtorchFrameParser::reset() {
  len_ = 0;
}

uint8_t AtorchFrameParser::checksumStd(const uint8_t* frame, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 2; i + 1 < len; ++i) {
    sum = static_cast<uint8_t>(sum + frame[i]);
  }
  return static_cast<uint8_t>(sum ^ 0x44);
}

uint8_t AtorchFrameParser::checksumXt369p(const uint8_t* frame, size_t len) {
  uint8_t sum = 0;
  for (size_t i = 3; i + 1 < len; ++i) {
    sum = static_cast<uint8_t>(sum + frame[i]);
  }
  return static_cast<uint8_t>(sum ^ 0x44);
}

uint32_t AtorchFrameParser::be24(const uint8_t* p) {
  return (static_cast<uint32_t>(p[0]) << 16) | (static_cast<uint32_t>(p[1]) << 8) |
         static_cast<uint32_t>(p[2]);
}

uint32_t AtorchFrameParser::be32(const uint8_t* p) {
  return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
         (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

uint16_t AtorchFrameParser::be16(const uint8_t* p) {
  return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

float AtorchFrameParser::resolveEnergyWh(uint32_t energyRaw, float capacityAh, float voltageV) {
  const float estimateWh =
      (capacityAh > 0.0f && voltageV > 0.5f) ? (capacityAh * voltageV) : 0.0f;
  if (energyRaw == 0u) {
    return estimateWh;
  }

  // Known DC scales: ESPHome DL24 (*10), NiceLabs (/100), some USB-like (/1000).
  const float candidates[3] = {static_cast<float>(energyRaw) * 10.0f,
                               static_cast<float>(energyRaw) / 100.0f,
                               static_cast<float>(energyRaw) / 1000.0f};

  if (estimateWh < 0.5f) {
    // Too little charge to score scales — prefer ESPHome DC (*10).
    return candidates[0];
  }

  float best = candidates[0];
  float bestDelta = candidates[0] > estimateWh ? (candidates[0] - estimateWh)
                                               : (estimateWh - candidates[0]);
  for (size_t i = 1; i < 3; ++i) {
    const float delta = candidates[i] > estimateWh ? (candidates[i] - estimateWh)
                                                   : (estimateWh - candidates[i]);
    if (delta < bestDelta) {
      bestDelta = delta;
      best = candidates[i];
    }
  }

  // If every register scale is absurd vs Ah×V, trust the physical estimate.
  if (best < estimateWh * 0.2f || best > estimateWh * 5.0f) {
    return estimateWh;
  }
  return best;
}

bool AtorchFrameParser::feed(const uint8_t* data, size_t length, WattmeterTelemetry& out) {
  bool produced = false;
  for (size_t i = 0; i < length; ++i) {
    if (len_ >= kBufCap) {
      std::memmove(buf_, buf_ + (kBufCap / 2), kBufCap / 2);
      len_ = kBufCap / 2;
    }
    buf_[len_++] = data[i];
    if (tryConsume(out)) {
      produced = true;
    }
  }
  return produced;
}

bool AtorchFrameParser::tryConsume(WattmeterTelemetry& out) {
  size_t start = 0;
  while (start + 2 < len_) {
    if (buf_[start] == 0xFF && buf_[start + 1] == 0x55) {
      break;
    }
    ++start;
  }
  if (start > 0) {
    std::memmove(buf_, buf_ + start, len_ - start);
    len_ -= start;
  }
  if (len_ < 3) {
    return false;
  }

  const uint8_t msg = buf_[2];
  size_t need = 0;
  if (msg == 0x01) {
    need = kReportLen;
  } else if (msg == 0x02) {
    need = 8;
  } else if (msg == 0x11) {
    need = 10;
  } else {
    std::memmove(buf_, buf_ + 1, len_ - 1);
    --len_;
    return false;
  }

  if (len_ < need) {
    return false;
  }

  if (msg != 0x01 || buf_[3] != 0x02) {
    std::memmove(buf_, buf_ + need, len_ - need);
    len_ -= need;
    return false;
  }

  const uint8_t* f = buf_;
  const uint8_t ck = f[need - 1];
  const bool ckOk = (ck == checksumStd(f, need)) || (ck == checksumXt369p(f, need));

  const float voltage = static_cast<float>(be24(f + 4)) * 0.1f;
  const float current = static_cast<float>(be24(f + 7)) * 0.001f;
  // DC capacity Ah (ESPHome layout). NiceLabs TS puts Wh at 0x0A instead — XT369P
  // matches capacity here (Ah tracks the LCD; Wh register at 0x0D is often 0).
  const float capacity = static_cast<float>(be24(f + 10)) * 0.01f;
  const uint32_t energyRaw = be32(f + 13);
  const float energy = resolveEnergyWh(energyRaw, capacity, voltage);
  const float price = static_cast<float>(be24(f + 17)) * 0.01f;
  const float temperature = static_cast<float>(be16(f + 24));
  const uint32_t runtime =
      static_cast<uint32_t>(be16(f + 26)) * 3600u + static_cast<uint32_t>(f[28]) * 60u + f[29];

  ++frameCount_;
#ifndef UNIT_TEST
  if ((frameCount_ % 30u) == 1u) {
    Serial.printf("DC raw cap=%02X%02X%02X wh=%02X%02X%02X%02X -> %.3fAh %.4fWh\n",
                  f[10], f[11], f[12], f[13], f[14], f[15], f[16],
                  static_cast<double>(capacity), static_cast<double>(energy));
  }
#endif
  out.valid = true;
  out.voltageV = voltage;
  out.currentA = current;
  out.powerW = voltage * current;
  out.capacityAh = capacity;
  out.energyWh = energy;
  out.temperatureC = temperature;
  out.pricePerKwh = price;
  out.runtimeS = runtime;
  out.backlightS = f[30];
  out.checksumOk = ckOk;
  out.frameCount = frameCount_;
#ifndef UNIT_TEST
  out.updatedAtMs = millis();
#else
  out.updatedAtMs = 0;
#endif

  std::memmove(buf_, buf_ + need, len_ - need);
  len_ -= need;
  return true;
}

}  // namespace xt369p::meter
