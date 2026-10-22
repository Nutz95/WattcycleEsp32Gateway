#pragma once

#include <cstdint>

#include "esp_lcd_types.h"
#include "src/lcd/st7701_lcd.h"
#include "src/touch/gt911_touch.h"

namespace wattcycle::panel {

/// Blanks the backlight and the panel scan, and wakes on touch or the side boot button.
class P4ScreenSleep {
 public:
  void begin(st7701_lcd& lcd, gt911_touch& touch, esp_lcd_panel_handle_t panel);
  /// Start the idle timer after the first frame is on screen.
  void arm();
  /// Wake from the boot button or a touch while the scan is stopped. True when this call woke the panel.
  bool pollWake();
  void blankIfIdle();
  /// True while a wake press must not activate the control under the finger.
  bool suppressTouch(bool pressed);
  bool screenOn() const;

 private:
  void setScreen(bool on);
  void noteActivity();
  void startButton();

  st7701_lcd* lcd_ = nullptr;
  gt911_touch* touch_ = nullptr;
  esp_lcd_panel_handle_t panel_ = nullptr;
  uint32_t lastActivityMs_ = 0;
  bool screenOn_ = true;
  bool swallow_ = false;
};

}  // namespace wattcycle::panel
