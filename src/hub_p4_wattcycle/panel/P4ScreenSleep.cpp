#include "P4ScreenSleep.h"

#include <Arduino.h>

#include "src/lcd/st7701_lcd.h"
#include "src/touch/gt911_touch.h"

#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"

namespace wattcycle::panel {
namespace {

constexpr uint32_t kBlankAfterMs = 60000;
constexpr gpio_num_t kSideBootButton = GPIO_NUM_35;

}  // namespace

void P4ScreenSleep::begin(st7701_lcd& lcd, gt911_touch& touch, esp_lcd_panel_handle_t panel) {
  lcd_ = &lcd;
  touch_ = &touch;
  panel_ = panel;
  startButton();
}

void P4ScreenSleep::arm() {
  lastActivityMs_ = millis();
  screenOn_ = true;
}

bool P4ScreenSleep::screenOn() const {
  return screenOn_;
}

void P4ScreenSleep::setScreen(bool on) {
  if (panel_ != nullptr) {
    esp_lcd_panel_disp_on_off(panel_, on);
  }
  if (lcd_ != nullptr) {
    lcd_->example_bsp_set_lcd_backlight(on ? 1u : 0u);
  }
  screenOn_ = on;
}

void P4ScreenSleep::noteActivity() {
  lastActivityMs_ = millis();
  if (!screenOn_) {
    setScreen(true);
  }
}

void P4ScreenSleep::blankIfIdle() {
  if (!screenOn_ || lastActivityMs_ == 0) {
    return;
  }
  if (static_cast<int32_t>(millis() - lastActivityMs_) >= static_cast<int32_t>(kBlankAfterMs)) {
    setScreen(false);
  }
}

void P4ScreenSleep::startButton() {
  gpio_config_t config = {};
  config.pin_bit_mask = 1ULL << kSideBootButton;
  config.mode = GPIO_MODE_INPUT;
  config.pull_up_en = GPIO_PULLUP_ENABLE;
  config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  config.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&config);
}

bool P4ScreenSleep::pollWake() {
  const bool wasDark = !screenOn_;
  if (gpio_get_level(kSideBootButton) == 0) {
    noteActivity();
  }
  if (!wasDark || touch_ == nullptr) {
    return false;
  }
  uint16_t x = 0;
  uint16_t y = 0;
  if (touch_->getTouch(&x, &y)) {
    noteActivity();
    swallow_ = true;
  }
  return wasDark && screenOn_;
}

bool P4ScreenSleep::suppressTouch(bool pressed) {
  if (swallow_) {
    if (pressed) {
      noteActivity();
    } else {
      swallow_ = false;
    }
    return true;
  }
  if (!pressed) {
    return false;
  }
  const bool wasDark = !screenOn_;
  noteActivity();
  if (wasDark) {
    swallow_ = true;
    return true;
  }
  return false;
}

}  // namespace wattcycle::panel
