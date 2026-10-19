#pragma once

#include <TFT_eSPI.h>

namespace boards::dongle_s3 {

/// 16-bpp dual-frame offscreen buffer.
/// Prefers internal RAM. present() bursts the frame over SPI (TFT_eSPI's
/// per-pixel push is what paints the diagonal wipe on ESP32-S3).
class DonglePsramDoubleBuffer {
 public:
  /// Bind to an already-constructed TFT_eSPI instance.
  explicit DonglePsramDoubleBuffer(TFT_eSPI& tft);

  /// Allocate 2×w×h RGB565 frames (DRAM first, PSRAM fallback).
  bool begin(int16_t width, int16_t height);

  /// Sprite currently selected for drawing (back buffer).
  TFT_eSprite& drawTarget();

  /// Burst-push the back buffer, then flip frames.
  void present();

  /// True after a successful begin().
  bool ready() const;

 private:
  TFT_eSPI& tft_;
  TFT_eSprite sprite_;
  int16_t width_ = 0;
  int16_t height_ = 0;
  int8_t drawFrame_ = 1;
  uint8_t presentLogs_ = 0;
  bool ready_ = false;
  bool inPsram_ = false;
};

}  // namespace boards::dongle_s3
