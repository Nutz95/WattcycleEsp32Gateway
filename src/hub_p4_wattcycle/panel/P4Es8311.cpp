#include "P4Es8311.h"

#include "driver/gpio.h"

namespace wattcycle::panel {
namespace {

constexpr uint8_t kAddress = 0x18;
constexpr gpio_num_t kAmpPin = GPIO_NUM_11;

}  // namespace

bool P4Es8311::writeReg(uint8_t reg, uint8_t value) {
  if (device_ == nullptr) {
    return false;
  }
  uint8_t payload[2] = {reg, value};
  return i2c_master_transmit(device_, payload, sizeof(payload), 100) == ESP_OK;
}

bool P4Es8311::readReg(uint8_t reg, uint8_t& value) {
  if (device_ == nullptr) {
    return false;
  }
  return i2c_master_transmit_receive(device_, &reg, 1, &value, 1, 100) == ESP_OK;
}

bool P4Es8311::begin(i2c_master_bus_handle_t bus) {
  if (bus == nullptr) {
    return false;
  }
  if (device_ != nullptr) {
    return true;
  }

  gpio_config_t amp = {};
  amp.pin_bit_mask = 1ULL << kAmpPin;
  amp.mode = GPIO_MODE_OUTPUT;
  gpio_config(&amp);
  gpio_set_level(kAmpPin, 0);

  i2c_device_config_t deviceConfig = {};
  deviceConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  deviceConfig.device_address = kAddress;
  deviceConfig.scl_speed_hz = 100000;
  if (i2c_master_bus_add_device(bus, &deviceConfig, &device_) != ESP_OK) {
    device_ = nullptr;
    return false;
  }

  // ADC open + start sequence from the board ES8311 driver, mic path only, PA off.
  bool ok = writeReg(0x00, 0x80);
  ok = writeReg(0x01, 0x30) && ok;
  ok = writeReg(0x02, 0x00) && ok;
  ok = writeReg(0x03, 0x10) && ok;
  ok = writeReg(0x16, 0x24) && ok;
  ok = writeReg(0x04, 0x10) && ok;
  ok = writeReg(0x05, 0x00) && ok;
  ok = writeReg(0x0B, 0x00) && ok;
  ok = writeReg(0x0C, 0x00) && ok;
  ok = writeReg(0x10, 0x1F) && ok;
  ok = writeReg(0x11, 0x7F) && ok;
  // MCLK in, slave, 16-bit I2S; clocks for 16 kHz with 4.096 MHz MCLK (256x).
  ok = writeReg(0x00, 0x80) && ok;
  ok = writeReg(0x01, 0x3F) && ok;
  ok = writeReg(0x02, 0x00) && ok;
  ok = writeReg(0x05, 0x11) && ok;
  ok = writeReg(0x03, 0x10) && ok;
  ok = writeReg(0x04, 0x10) && ok;
  ok = writeReg(0x07, 0x00) && ok;
  ok = writeReg(0x08, 0xFF) && ok;
  ok = writeReg(0x06, 0x04) && ok;
  ok = writeReg(0x13, 0x10) && ok;
  ok = writeReg(0x1B, 0x0A) && ok;
  ok = writeReg(0x1C, 0x6A) && ok;
  ok = writeReg(0x44, 0x08) && ok;
  uint8_t adcIface = 0;
  if (readReg(0x0A, adcIface)) {
    adcIface = static_cast<uint8_t>((adcIface & 0xBF) | 0x0C);
    ok = writeReg(0x0A, adcIface) && ok;
  }
  ok = writeReg(0x17, 0xFF) && ok;
  ok = writeReg(0x0E, 0x02) && ok;
  ok = writeReg(0x14, 0x1A) && ok;
  ok = writeReg(0x0D, 0x01) && ok;
  ok = writeReg(0x15, 0x40) && ok;
  if (!ok) {
    end();
  }
  return ok;
}

void P4Es8311::end() {
  if (device_ == nullptr) {
    return;
  }
  writeReg(0x17, 0x00);
  writeReg(0x0E, 0xFF);
  writeReg(0x14, 0x00);
  writeReg(0x0D, 0xFA);
  writeReg(0x00, 0x1F);
  writeReg(0x01, 0x00);
  i2c_master_bus_rm_device(device_);
  device_ = nullptr;
}

}  // namespace wattcycle::panel
