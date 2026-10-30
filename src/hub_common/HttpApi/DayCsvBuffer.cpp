#include "HttpApi/DayCsvBuffer.h"

#include <cstdlib>
#include <cstring>

namespace wattcycle::web {
namespace {

bool grow(uint8_t*& data, size_t& capacity, size_t needed) {
  if (needed <= capacity) {
    return true;
  }
  size_t next = capacity == 0 ? 4096 : capacity;
  while (next < needed) {
    next *= 2;
  }
  uint8_t* grown = static_cast<uint8_t*>(std::realloc(data, next));
  if (grown == nullptr) {
    return false;
  }
  data = grown;
  capacity = next;
  return true;
}

}  // namespace

DayCsvBuffer::~DayCsvBuffer() {
  std::free(data_);
  data_ = nullptr;
}

bool DayCsvBuffer::reserve(size_t bytes) {
  return grow(data_, capacity_, bytes);
}

bool DayCsvBuffer::append(const uint8_t* data, size_t length) {
  if (data == nullptr || length == 0) {
    return true;
  }
  if (!grow(data_, capacity_, used_ + length)) {
    return false;
  }
  std::memcpy(data_ + used_, data, length);
  used_ += length;
  return true;
}

void DayCsvBuffer::clear() {
  used_ = 0;
}

}  // namespace wattcycle::web
