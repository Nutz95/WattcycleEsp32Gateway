#pragma once

#include "P4AdpcmEncoder.h"

#include <ESP_I2S.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include <cstddef>
#include <cstdint>

namespace wattcycle::panel {

/// Drains I2S on core 0 so JPEG work on core 1 does not drop microphone samples.
class P4MicPump {
 public:
  void start(I2SClass& microphone);
  void setRunning(bool running);
  void clear();
  /// Latest mic level, 0..100, computed while samples are encoded.
  int peak() const;
  size_t take(uint8_t* dst, size_t capacity);

 private:
  static void taskMain(void* context);
  void run();
  void notePeak(const int16_t* samples, size_t count);
  void push(const uint8_t* data, size_t length);

  static constexpr size_t kCapacity = 48 * 1024;
  I2SClass* microphone_ = nullptr;
  uint8_t* ring_ = nullptr;
  size_t head_ = 0;
  size_t tail_ = 0;
  size_t used_ = 0;
  volatile bool running_ = false;
  volatile int peak_ = 0;
  bool resetEncoder_ = false;
  bool started_ = false;
  P4AdpcmEncoder encoder_;
  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
};

}  // namespace wattcycle::panel
