#pragma once

#include <cstddef>
#include <cstdint>

#include "Meter/Models/WattmeterTelemetry.h"

namespace xt369p::meter {

/// Incremental FF55 frame reassembly + DC report decode (ESPHome DC layout).
class AtorchFrameParser {
 public:
  /// Feed raw SPP bytes; returns true when a new DC reading was produced.
  bool feed(const uint8_t* data, size_t length, WattmeterTelemetry& out);

  void reset();

 private:
  static constexpr size_t kReportLen = 36;
  static constexpr size_t kBufCap = 96;

  static uint8_t checksumStd(const uint8_t* frame, size_t len);
  static uint8_t checksumXt369p(const uint8_t* frame, size_t len);
  static uint32_t be24(const uint8_t* p);
  static uint32_t be32(const uint8_t* p);
  static uint16_t be16(const uint8_t* p);
  bool tryConsume(WattmeterTelemetry& out);

  uint8_t buf_[kBufCap] = {};
  size_t len_ = 0;
  uint32_t frameCount_ = 0;
};

}  // namespace xt369p::meter
