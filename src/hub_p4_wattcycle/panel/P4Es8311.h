#pragma once

#include "driver/i2c_master.h"

namespace wattcycle::panel {

/// ADC-only bring-up for the panel ES8311 (PA stays off).
class P4Es8311 {
 public:
  bool begin(i2c_master_bus_handle_t bus);
  void end();

 private:
  bool writeReg(uint8_t reg, uint8_t value);
  bool readReg(uint8_t reg, uint8_t& value);
  i2c_master_dev_handle_t device_ = nullptr;
};

}  // namespace wattcycle::panel
