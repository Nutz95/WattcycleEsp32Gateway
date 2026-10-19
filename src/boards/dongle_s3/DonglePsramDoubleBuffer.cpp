#include "boards/dongle_s3/DonglePsramDoubleBuffer.h"

#include <Arduino.h>

namespace boards::dongle_s3 {

DonglePsramDoubleBuffer::DonglePsramDoubleBuffer(TFT_eSPI& tft) : tft_(tft), sprite_(&tft) {}

bool DonglePsramDoubleBuffer::begin(int16_t width, int16_t height) {
  if (width < 1 || height < 1) {
    return false;
  }

  sprite_.setColorDepth(16);

  // Prefer DRAM: a PSRAM read during the SPI burst stretches the visible wipe.
  tft_.setAttribute(PSRAM_ENABLE, false);
  inPsram_ = false;
  if (sprite_.createSprite(width, height, 2) == nullptr) {
    tft_.setAttribute(PSRAM_ENABLE, true);
    if (sprite_.createSprite(width, height, 2) == nullptr) {
      Serial.printf("[dongle-fb] createSprite(%d,%d,2) failed heap=%u psram=%u\n", width, height,
                    ESP.getFreeHeap(), ESP.getFreePsram());
      return false;
    }
    inPsram_ = true;
  }

  width_ = width;
  height_ = height;
  drawFrame_ = 1;
  sprite_.frameBuffer(drawFrame_);
  ready_ = true;
  Serial.printf("[dongle-fb] OK %dx%d x2 mem=%s heap=%u psram=%u\n", width, height,
                inPsram_ ? "PSRAM" : "DRAM", ESP.getFreeHeap(), ESP.getFreePsram());
  return true;
}

TFT_eSprite& DonglePsramDoubleBuffer::drawTarget() {
  return sprite_;
}

void DonglePsramDoubleBuffer::present() {
  if (!ready_) {
    return;
  }

  uint16_t* pixels = static_cast<uint16_t*>(sprite_.getPointer());
  if (pixels == nullptr) {
    return;
  }

  // Match TFT_eSprite::pushSprite: swapBytes=false + pushPixels.
  // Do NOT byte-swap then SPI.writeBytes — that inverts mid-tones (gray→pink,
  // yellow→violet). Do NOT SPI.writePixels (MSB_PIX_SET) either — same trap.
  const uint32_t startedUs = micros();
  const uint32_t pixelCount =
      static_cast<uint32_t>(width_) * static_cast<uint32_t>(height_);

  tft_.startWrite();
  tft_.setAddrWindow(0, 0, width_, height_);
  const bool oldSwap = tft_.getSwapBytes();
  tft_.setSwapBytes(false);
  tft_.pushPixels(pixels, pixelCount);
  tft_.setSwapBytes(oldSwap);
  tft_.endWrite();

  if (presentLogs_ < 4) {
    Serial.printf("[dongle-fb] burst present %u us\n",
                  static_cast<unsigned>(micros() - startedUs));
    ++presentLogs_;
  }

  drawFrame_ = (drawFrame_ == 1) ? 2 : 1;
  sprite_.frameBuffer(drawFrame_);
}

bool DonglePsramDoubleBuffer::ready() const {
  return ready_;
}

}  // namespace boards::dongle_s3
