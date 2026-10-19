#pragma once

#include "boards/dongle_s3/DongleEspHealthLines.h"
#include "boards/dongle_s3/DonglePsramDoubleBuffer.h"

#include <TFT_eSPI.h>

#include <cstdint>

namespace boards::dongle_s3 {

/// Owns TFT + double-buffer chrome for Pocket/LilyGO 160x80 status UIs.
class DonglePanelShell {
 public:
  /// Init TFT, allocate frames, draw splash. Returns false if framebuffer fails.
  bool begin(const char* logTag, const char* splashText);
  /// Toggle backlight GPIO or ST7735 DISPOFF/ON when hard-wired.
  void setBacklight(bool on);
  /// True when panel output is considered on.
  bool isBacklightOn() const;
  /// True after a successful begin().
  bool ready() const;
  /// Offscreen canvas for product pages.
  TFT_eSprite& canvas();
  /// Clear + banner + page dots.
  void drawChrome(const char* title, uint8_t pageIndex, uint8_t pageCount);
  /// Shared ESP page (MAC / CPU / heap / chip temp).
  void drawEspPage(const char* title, uint8_t pageIndex, uint8_t pageCount,
                   const DongleEspHealthLines& lines);
  /// Burst the back-buffer to the panel.
  void present();

 private:
  static constexpr uint8_t kSt7735DispOff = 0x28;
  static constexpr uint8_t kSt7735DispOn = 0x29;

  TFT_eSPI tft_{};
  DonglePsramDoubleBuffer frame_{tft_};
  bool ready_ = false;
  bool backlightOn_ = true;
};

}  // namespace boards::dongle_s3
