#pragma once

#include <cstddef>
#include <cstdint>

namespace wattcycle::web {

/// Heap buffer that collects a day CSV before optional gzip.
class DayCsvBuffer {
 public:
  DayCsvBuffer() = default;
  ~DayCsvBuffer();

  DayCsvBuffer(const DayCsvBuffer&) = delete;
  DayCsvBuffer& operator=(const DayCsvBuffer&) = delete;

  bool reserve(size_t bytes);
  bool append(const uint8_t* data, size_t length);
  const uint8_t* data() const { return data_; }
  size_t size() const { return used_; }
  void clear();

 private:
  uint8_t* data_ = nullptr;
  size_t used_ = 0;
  size_t capacity_ = 0;
};

}  // namespace wattcycle::web
