#include "P4MicPump.h"

#include <Arduino.h>

#include <esp_heap_caps.h>

namespace wattcycle::panel {

void P4MicPump::start(I2SClass& microphone) {
  microphone_ = &microphone;
  if (ring_ == nullptr) {
    ring_ = static_cast<uint8_t*>(heap_caps_malloc(kCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }
  if (started_) {
    return;
  }
  started_ = true;
  xTaskCreatePinnedToCore(&P4MicPump::taskMain, "micPump", 6144, this, 3, nullptr, 0);
}

void P4MicPump::setRunning(bool running) {
  running_ = running;
}

void P4MicPump::clear() {
  portENTER_CRITICAL(&mux_);
  head_ = 0;
  tail_ = 0;
  used_ = 0;
  resetEncoder_ = true;
  portEXIT_CRITICAL(&mux_);
  peak_ = 0;
}

int P4MicPump::peak() const {
  return peak_;
}

size_t P4MicPump::take(uint8_t* dst, size_t capacity) {
  if (dst == nullptr || ring_ == nullptr || capacity < 4) {
    return 0;
  }
  portENTER_CRITICAL(&mux_);
  size_t count = used_;
  if (count > capacity) {
    count = capacity;
  }
  count -= count % 4;
  for (size_t index = 0; index < count; ++index) {
    dst[index] = ring_[tail_];
    tail_ += 1;
    if (tail_ == kCapacity) {
      tail_ = 0;
    }
  }
  used_ -= count;
  portEXIT_CRITICAL(&mux_);
  return count;
}

void P4MicPump::push(const uint8_t* data, size_t length) {
  if (data == nullptr || ring_ == nullptr || length == 0) {
    return;
  }
  portENTER_CRITICAL(&mux_);
  while (used_ + length > kCapacity && used_ >= P4AdpcmEncoder::kFrameBytes) {
    tail_ += P4AdpcmEncoder::kFrameBytes;
    if (tail_ >= kCapacity) {
      tail_ -= kCapacity;
    }
    used_ -= P4AdpcmEncoder::kFrameBytes;
  }
  if (used_ + length > kCapacity) {
    portEXIT_CRITICAL(&mux_);
    return;
  }
  for (size_t index = 0; index < length; ++index) {
    ring_[head_] = data[index];
    head_ += 1;
    if (head_ == kCapacity) {
      head_ = 0;
    }
    used_ += 1;
  }
  portEXIT_CRITICAL(&mux_);
}

void P4MicPump::taskMain(void* context) {
  static_cast<P4MicPump*>(context)->run();
}

void P4MicPump::notePeak(const int16_t* samples, size_t count) {
  int peak = 0;
  for (size_t index = 0; index < count; ++index) {
    const int value = samples[index] < 0 ? -samples[index] : samples[index];
    if (value > peak) {
      peak = value;
    }
  }
  peak_ = (peak * 100) / 32768;
}

void P4MicPump::run() {
  uint8_t chunk[1024];
  uint8_t encoded[P4AdpcmEncoder::kFrameBytes * 2];
  for (;;) {
    if (!running_ || microphone_ == nullptr || ring_ == nullptr) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    if (microphone_->available() < 4) {
      vTaskDelay(pdMS_TO_TICKS(2));
      continue;
    }
    const size_t got = microphone_->readBytes(reinterpret_cast<char*>(chunk), sizeof(chunk));
    if (got < 4) {
      continue;
    }
    bool resetEncoder = false;
    portENTER_CRITICAL(&mux_);
    resetEncoder = resetEncoder_;
    resetEncoder_ = false;
    portEXIT_CRITICAL(&mux_);
    if (resetEncoder) {
      encoder_.reset();
    }
    const auto* samples = reinterpret_cast<const int16_t*>(chunk);
    const size_t count = got / sizeof(int16_t);
    notePeak(samples, count);
    const size_t wrote = encoder_.encode(samples, count / 2, encoded, sizeof(encoded));
    if (wrote > 0) {
      push(encoded, wrote);
    }
  }
}

}  // namespace wattcycle::panel
