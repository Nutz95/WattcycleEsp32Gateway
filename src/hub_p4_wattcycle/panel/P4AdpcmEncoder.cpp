#include "P4AdpcmEncoder.h"

namespace wattcycle::panel {
namespace {

constexpr int kStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,
    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,
    107,   118,   130,   143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,
    408,   449,   494,   544,   598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,
    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
    5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767};

constexpr int kIndexTable[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

}  // namespace

void P4AdpcmEncoder::reset() {
  predictor_ = 0;
  index_ = 0;
  pendingCount_ = 0;
}

uint8_t P4AdpcmEncoder::encodeSample(int16_t sample) {
  const int step = kStepTable[index_];
  int diff = static_cast<int>(sample) - predictor_;
  uint8_t nibble = 0;
  if (diff < 0) {
    nibble = 8;
    diff = -diff;
  }
  if (diff >= step) {
    nibble = static_cast<uint8_t>(nibble | 4);
    diff -= step;
  }
  if (diff >= (step >> 1)) {
    nibble = static_cast<uint8_t>(nibble | 2);
    diff -= step >> 1;
  }
  if (diff >= (step >> 2)) {
    nibble = static_cast<uint8_t>(nibble | 1);
  }
  int delta = step >> 3;
  if ((nibble & 4) != 0) {
    delta += step;
  }
  if ((nibble & 2) != 0) {
    delta += step >> 1;
  }
  if ((nibble & 1) != 0) {
    delta += step >> 2;
  }
  predictor_ += ((nibble & 8) != 0) ? -delta : delta;
  if (predictor_ > 32767) {
    predictor_ = 32767;
  }
  if (predictor_ < -32768) {
    predictor_ = -32768;
  }
  index_ += kIndexTable[nibble];
  if (index_ < 0) {
    index_ = 0;
  }
  if (index_ > 88) {
    index_ = 88;
  }
  return nibble;
}

size_t P4AdpcmEncoder::emit(uint8_t* dst, size_t capacity) {
  if (dst == nullptr || capacity < kFrameBytes || pendingCount_ < kFrameSamples) {
    return 0;
  }
  dst[0] = 0xAD;
  dst[1] = static_cast<uint8_t>(index_);
  dst[2] = static_cast<uint8_t>(predictor_ & 0xFF);
  dst[3] = static_cast<uint8_t>((predictor_ >> 8) & 0xFF);
  for (size_t index = 0; index < kFrameSamples; index += 2) {
    const uint8_t low = encodeSample(pending_[index]);
    const uint8_t high = encodeSample(pending_[index + 1]);
    dst[4 + (index / 2)] = static_cast<uint8_t>(low | (high << 4));
  }
  pendingCount_ = 0;
  return kFrameBytes;
}

size_t P4AdpcmEncoder::encode(const int16_t* stereo, size_t pairCount, uint8_t* dst, size_t capacity) {
  if (stereo == nullptr || dst == nullptr || pairCount == 0) {
    return 0;
  }
  size_t written = 0;
  for (size_t index = 0; index < pairCount; ++index) {
    if (pendingCount_ == kFrameSamples) {
      if (written + kFrameBytes > capacity) {
        break;
      }
      written += emit(dst + written, capacity - written);
    }
    const int mixed = (static_cast<int>(stereo[index * 2]) + static_cast<int>(stereo[index * 2 + 1])) / 2;
    pending_[pendingCount_] = static_cast<int16_t>(mixed);
    pendingCount_ += 1;
  }
  if (pendingCount_ == kFrameSamples && written + kFrameBytes <= capacity) {
    written += emit(dst + written, capacity - written);
  }
  return written;
}

}  // namespace wattcycle::panel
