#include "P4LiveView.h"

#include "P4LiveHttp.h"

#include <Arduino.h>

#include <esp_heap_caps.h>

#include <cstdio>
#include <cstring>

#include "driver/i2c_master.h"
#include "esp_vfs.h"

namespace wattcycle::panel {
namespace {

constexpr uint8_t kOv02c10Addr = 0x36;
constexpr int kPanelPreviewW = 320;
constexpr int kPanelPreviewH = 180;

}  // namespace

bool P4LiveView::probeSensor() {
  if (bus_ == nullptr) {
    return false;
  }
  i2c_device_config_t deviceConfig = {};
  deviceConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  deviceConfig.device_address = kOv02c10Addr;
  deviceConfig.scl_speed_hz = 100000;
  i2c_master_dev_handle_t device = nullptr;
  if (i2c_master_bus_add_device(bus_, &deviceConfig, &device) != ESP_OK) {
    return false;
  }
  uint8_t high = 0;
  uint8_t low = 0;
  const uint8_t regHigh[2] = {0x30, 0x0A};
  const uint8_t regLow[2] = {0x30, 0x0B};
  const esp_err_t highErr = i2c_master_transmit_receive(device, regHigh, sizeof(regHigh), &high, 1, 100);
  const esp_err_t lowErr = i2c_master_transmit_receive(device, regLow, sizeof(regLow), &low, 1, 100);
  i2c_master_bus_rm_device(device);
  const unsigned pid = (static_cast<unsigned>(high) << 8) | low;
  std::snprintf(probeNote_, sizeof(probeNote_), "probe pid=0x%04x high=%d low=%d", pid,
                static_cast<int>(highErr), static_cast<int>(lowErr));
  Serial.printf("live cam: %s (expect 0x5602)\n", probeNote_);
  return highErr == ESP_OK && lowErr == ESP_OK;
}

bool P4LiveView::beginVideo(ESPVideoCSIConfigClass& csi) {
  if (video.begin(csi)) {
    return true;
  }
  std::snprintf(cameraFail_, sizeof(cameraFail_), "video_begin");
  Serial.println("live cam: video.begin failed");
  return false;
}

bool P4LiveView::openCamera() {
  if (cameraOpen_ || bus_ == nullptr) {
    return cameraOpen_;
  }
  probeSensor();
  if (!nullVfsReleased_) {
    if (esp_vfs_unregister("/dev/null") == ESP_OK) {
      Serial.println("live cam: released /dev/null VFS slot");
    }
    nullVfsReleased_ = true;
  }
  ESPVideoCamConfigClass cam;
  cam.begin(bus_, 100000);
  ESPVideoCSIConfigClass csi;
  // The panel LCD already owns MIPI LDO channel 3.
  csi.begin(cam, true);
  Serial.println("live cam: video.begin");
  if (!beginVideo(csi)) {
    return false;
  }
  if (!capture.begin(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, 2)) {
    std::snprintf(cameraFail_, sizeof(cameraFail_), "capture_begin");
    Serial.println("live cam: capture.begin failed");
    video.end();
    return false;
  }
  if (!capture.setFormat(ESP_VIDEO_FORMAT_RGB565)) {
    std::snprintf(cameraFail_, sizeof(cameraFail_), "set_rgb565");
    Serial.println("live cam: setFormat RGB565 failed");
    capture.end();
    video.end();
    return false;
  }
  if (!capture.startCapture()) {
    std::snprintf(cameraFail_, sizeof(cameraFail_), "start_capture");
    Serial.println("live cam: startCapture failed");
    capture.end();
    video.end();
    return false;
  }
  cameraFail_[0] = '\0';
  cameraOpen_ = true;
  Serial.printf("live cam: open ok %ux%u\n", capture.getWidth(), capture.getHeight());
  return true;
}

void P4LiveView::closeCamera() {
  if (!cameraOpen_) {
    return;
  }
  capture.end();
  video.end();
  cameraOpen_ = false;
  jpegOutSize_ = 0;
}

bool P4LiveView::openMic() {
  if (micOpen_) {
    return true;
  }
  if (!codec_.begin(bus_)) {
    return false;
  }
  microphone.setPins(12, 10, 9, 48, 13);
  micOpen_ = microphone.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  if (!micOpen_) {
    codec_.end();
    return false;
  }
  micPump_.setRunning(true);
  return true;
}

void P4LiveView::closeMic() {
  if (!micOpen_) {
    return;
  }
  micPump_.setRunning(false);
  delay(30);
  micPump_.clear();
  microphone.end();
  codec_.end();
  micOpen_ = false;
}

void P4LiveView::scaleRgb565(const uint8_t* src, int srcW, int srcH, uint16_t* dst, int dstW, int dstH) {
  if (src == nullptr || dst == nullptr || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) {
    return;
  }
  const auto* pixels = reinterpret_cast<const uint16_t*>(src);
  for (int y = 0; y < dstH; ++y) {
    const int sy = (y * srcH) / dstH;
    for (int x = 0; x < dstW; ++x) {
      const int sx = (x * srcW) / dstW;
      dst[y * dstW + x] = pixels[sy * srcW + sx];
    }
  }
}

int P4LiveView::readMicPeak() {
  if (!micOpen_) {
    return 0;
  }
  return micPump_.peak();
}

void P4LiveView::pollPanel() {
  if (!cameraOpen_) {
    const uint32_t now = millis();
    if (nextOpenMs_ != 0 && static_cast<int32_t>(now - nextOpenMs_) < 0) {
      return;
    }
    nextOpenMs_ = now + 3000;
    if (!openCamera()) {
      panelFrameReady_ = false;
      Serial.printf("live cam: open failed %s %s\n", cameraFail_, probeNote_);
      return;
    }
  }
  if (!micOpen_) {
    openMic();
  }
  ESPVideoBufferClass frame = capture.captureBuffer();
  if (!frame.valid() || frame.formatType() != ESP_VIDEO_FORMAT_RGB565) {
    return;
  }
  const size_t bytes = static_cast<size_t>(kPanelPreviewW * kPanelPreviewH) * sizeof(uint16_t);
  if (panelRgb_ == nullptr) {
    panelRgb_ = static_cast<uint16_t*>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (panelRgb_ == nullptr) {
      return;
    }
  }
  scaleRgb565(frame.data(), static_cast<int>(frame.getWidth()), static_cast<int>(frame.getHeight()), panelRgb_,
              kPanelPreviewW, kPanelPreviewH);
  const int peak = readMicPeak();
  portENTER_CRITICAL(&panelMux_);
  panelW_ = kPanelPreviewW;
  panelH_ = kPanelPreviewH;
  panelPeak_ = peak;
  panelFrameReady_ = true;
  panelFrames_ += 1;
  portEXIT_CRITICAL(&panelMux_);
  if (panelFrames_ == 1 || (panelFrames_ % 30) == 0) {
    Serial.printf("live cam: frame %u peak=%d\n", static_cast<unsigned>(panelFrames_), peak);
  }
}

void P4LiveView::pollJpeg() {
  if (claim_ == P4LiveClaim::Panel) {
    rejectBusy(jpegServer, "panel_busy");
    return;
  }
  if (!liveAcceptAuthorized(jpegServer, jpegClient, *auth_, jpegOrigin_, sizeof(jpegOrigin_))) {
    if (claim_ == P4LiveClaim::Web) {
      claim_ = P4LiveClaim::None;
    }
    wantsBlank_ = false;
    jpegStarted_ = false;
    closeCamera();
    return;
  }
  claim_ = P4LiveClaim::Web;
  wantsBlank_ = true;
  if (!cameraOpen_ && !openCamera()) {
    liveSendStatus(jpegClient, 503, cameraFail_[0] != '\0' ? cameraFail_ : "camera_unavailable");
    jpegClient.stop();
    jpegStarted_ = false;
    wantsBlank_ = false;
    claim_ = P4LiveClaim::None;
    return;
  }
  if (!jpegStarted_) {
    char cors[160] = {};
    liveFillCors(jpegOrigin_, cors, sizeof(cors));
    char head[320] = {};
    std::snprintf(head, sizeof(head),
                  "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                  "Cache-Control: no-store\r\nConnection: close\r\n%s\r\n",
                  cors);
    if (!liveWriteAll(jpegClient, reinterpret_cast<const uint8_t*>(head), std::strlen(head))) {
      jpegClient.stop();
      jpegStarted_ = false;
      return;
    }
    jpegStarted_ = true;
  }
  ESPVideoBufferClass frame = capture.captureBuffer();
  if (!frame.valid() || frame.formatType() != ESP_VIDEO_FORMAT_RGB565) {
    return;
  }
  if (!encodeStream(frame.data(), static_cast<int>(frame.getWidth()), static_cast<int>(frame.getHeight()))) {
    return;
  }
  char part[96] = {};
  std::snprintf(part, sizeof(part),
                "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                static_cast<unsigned>(jpegOutSize_));
  const bool wrote = liveWriteAll(jpegClient, reinterpret_cast<const uint8_t*>(part), std::strlen(part)) &&
                     liveWriteAll(jpegClient, jpegOut_, jpegOutSize_) &&
                     liveWriteAll(jpegClient, reinterpret_cast<const uint8_t*>("\r\n"), 2);
  if (!wrote) {
    jpegClient.stop();
    jpegStarted_ = false;
  }
}

void P4LiveView::pollMic() {
  if (claim_ == P4LiveClaim::Panel) {
    rejectBusy(micServer, "panel_busy");
    return;
  }
  if (!liveAcceptAuthorized(micServer, micClient, *auth_, micOrigin_, sizeof(micOrigin_))) {
    micStarted_ = false;
    if (claim_ != P4LiveClaim::Web) {
      closeMic();
    }
    return;
  }
  if (!micOpen_ && !openMic()) {
    liveSendStatus(micClient, 503, "mic_unavailable");
    micClient.stop();
    micStarted_ = false;
    return;
  }
  if (!micStarted_) {
    char cors[160] = {};
    liveFillCors(micOrigin_, cors, sizeof(cors));
    char head[320] = {};
    std::snprintf(head, sizeof(head),
                  "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\n"
                  "Cache-Control: no-store\r\nConnection: close\r\n%s\r\n",
                  cors);
    if (!liveWriteAll(micClient, reinterpret_cast<const uint8_t*>(head), std::strlen(head))) {
      micClient.stop();
      micStarted_ = false;
      return;
    }
    micStarted_ = true;
  }
  int drained = 0;
  while (drained < 8192) {
    uint8_t chunk[1024] = {};
    const size_t got = micPump_.take(chunk, sizeof(chunk));
    if (got == 0) {
      break;
    }
    drained += static_cast<int>(got);
    if (!liveWriteAll(micClient, chunk, got)) {
      micClient.stop();
      micStarted_ = false;
      return;
    }
  }
}

}  // namespace wattcycle::panel
