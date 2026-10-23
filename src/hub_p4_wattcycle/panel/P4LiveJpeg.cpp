#include "P4LiveView.h"

#include <Arduino.h>

#include <esp_heap_caps.h>
#include <esp_jpeg_enc.h>

namespace wattcycle::panel {
namespace {

constexpr int kStreamW = 640;
constexpr int kStreamH = 360;
constexpr uint32_t kJpegQuality = 95;

uint8_t* allocEncoderBuffer(uint8_t* current, size_t& capacity, size_t need,
                            jpeg_enc_buffer_alloc_direction_t direction) {
  if (current != nullptr && capacity >= need) {
    return current;
  }
  if (current != nullptr) {
    heap_caps_free(current);
  }
  jpeg_encode_memory_alloc_cfg_t config = {};
  config.buffer_direction = direction;
  size_t allocated = 0;
  auto* buffer = static_cast<uint8_t*>(jpeg_alloc_encoder_mem(need, &config, &allocated));
  capacity = buffer == nullptr ? 0 : allocated;
  return buffer;
}

}  // namespace

bool P4LiveView::encodeHw(const uint8_t* rgb, size_t rgbSize, uint16_t width, uint16_t height) {
  if (hwUnsupported_ || jpegOut_ == nullptr) {
    return false;
  }
  if (hwJpeg_ == nullptr) {
    jpeg_encode_engine_cfg_t engine = {};
    engine.timeout_ms = 200;
    if (jpeg_new_encoder_engine(&engine, &hwJpeg_) != ESP_OK) {
      hwJpeg_ = nullptr;
      hwUnsupported_ = true;
      Serial.println("live cam: hardware jpeg unavailable");
      return false;
    }
    Serial.printf("live cam: hardware jpeg %ux%u q%u\n", static_cast<unsigned>(width),
                  static_cast<unsigned>(height), static_cast<unsigned>(kJpegQuality));
  }
  jpeg_encode_cfg_t cfg = {};
  cfg.height = height;
  cfg.width = width;
  cfg.src_type = JPEG_ENCODE_IN_FORMAT_RGB565;
  cfg.sub_sample = JPEG_DOWN_SAMPLING_YUV420;
  cfg.image_quality = kJpegQuality;
  cfg.pixel_reverse = false;
  uint32_t outSize = 0;
  const esp_err_t err = jpeg_encoder_process(hwJpeg_, &cfg, rgb, static_cast<uint32_t>(rgbSize), jpegOut_,
                                             static_cast<uint32_t>(jpegOutCapacity_), &outSize);
  if (err != ESP_OK || outSize == 0) {
    if (!hwFrameFailed_) {
      hwFrameFailed_ = true;
      Serial.printf("live cam: hardware jpeg err=%d\n", static_cast<int>(err));
    }
    return false;
  }
  jpegOutSize_ = outSize;
  return true;
}

bool P4LiveView::encodeJpeg(const uint8_t* rgb, size_t rgbSize, uint16_t width, uint16_t height) {
  jpegOut_ = allocEncoderBuffer(jpegOut_, jpegOutCapacity_, static_cast<size_t>(width) * height,
                                JPEG_ENC_ALLOC_OUTPUT_BUFFER);
  if (jpegOut_ == nullptr) {
    return false;
  }
  if (encodeHw(rgb, rgbSize, width, height)) {
    return true;
  }
  jpeg_enc_config_t config = DEFAULT_JPEG_ENC_CONFIG();
  config.width = width;
  config.height = height;
  config.src_type = JPEG_PIXEL_FORMAT_RGB565_LE;
  config.subsampling = JPEG_SUBSAMPLE_420;
  config.quality = static_cast<uint8_t>(kJpegQuality);
  jpeg_enc_handle_t encoder = nullptr;
  if (jpeg_enc_open(&config, &encoder) != JPEG_ERR_OK) {
    return false;
  }
  int outSize = 0;
  const jpeg_error_t err =
      jpeg_enc_process(encoder, rgb, static_cast<int>(rgbSize), jpegOut_, static_cast<int>(jpegOutCapacity_),
                       &outSize);
  jpeg_enc_close(encoder);
  if (err != JPEG_ERR_OK || outSize <= 0) {
    return false;
  }
  jpegOutSize_ = static_cast<size_t>(outSize);
  return true;
}

bool P4LiveView::encodeStream(const uint8_t* src, int srcW, int srcH) {
  const size_t streamBytes = static_cast<size_t>(kStreamW * kStreamH) * sizeof(uint16_t);
  if (streamRgb_ == nullptr) {
    streamRgb_ = static_cast<uint16_t*>(
        heap_caps_aligned_alloc(16, streamBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }
  if (streamRgb_ == nullptr || src == nullptr) {
    return false;
  }
  scaleRgb565(src, srcW, srcH, streamRgb_, kStreamW, kStreamH);
  return encodeJpeg(reinterpret_cast<const uint8_t*>(streamRgb_), streamBytes, kStreamW, kStreamH);
}

}  // namespace wattcycle::panel
