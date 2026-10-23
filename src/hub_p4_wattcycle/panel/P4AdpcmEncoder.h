#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::panel {

/// IMA ADPCM frames for the web microphone. Each frame is 320 mono samples.
class P4AdpcmEncoder {
 public:
  static constexpr size_t kFrameBytes = 164;

  void reset();
  /// Mix stereo pairs to mono and append complete frames to `dst`.
  size_t encode(const int16_t* stereo, size_t pairCount, uint8_t* dst, size_t capacity);

 private:
  uint8_t encodeSample(int16_t sample);
  size_t emit(uint8_t* dst, size_t capacity);

  static constexpr size_t kFrameSamples = 320;
  int32_t predictor_ = 0;
  int index_ = 0;
  int16_t pending_[kFrameSamples] = {};
  size_t pendingCount_ = 0;
};

}  // namespace wattcycle::panel
