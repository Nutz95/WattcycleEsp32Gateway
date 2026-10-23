#pragma once

#include "Auth/IAuthSessionService.h"
#include "P4Es8311.h"
#include "P4LiveClaim.h"
#include "P4MicPump.h"

#include "driver/i2c_master.h"
#include "driver/jpeg_encode.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include <ESP_I2S.h>
#include <ESP_Video.h>
#include <WiFi.h>

#include <cstddef>
#include <cstdint>

namespace wattcycle::panel {

/// Front camera + mic. The live task owns the devices and the claim.
class P4LiveView {
 public:
  void begin(i2c_master_bus_handle_t bus, auth::IAuthSessionService& auth);
  /// True while a browser holds the camera stream. The panel should stay dark.
  bool wantsBlank() const;
  /// Ask the live task to take or release the on-panel preview.
  void requestPanel(bool want);
  /// True while the Camera page owns the pipeline.
  bool panelClaimed() const;
  /// Last open failure token (empty when healthy).
  const char* cameraFail() const;
  /// Copy one scaled RGB565 preview and a 0..100 mic peak. Returns false if not ready.
  bool pullPanelFrame(uint16_t* rgb565, int outWidth, int outHeight, int& micPeak);

 private:
  static constexpr uint16_t kJpegPort = 47690;
  static constexpr uint16_t kMicPort = 47691;

  static void taskMain(void* context);
  void run();
  void applyRequest();
  void dropServers();
  bool ensureServers();
  void rejectBusy(WiFiServer& server, const char* reason);
  void pollJpeg();
  void pollMic();
  void pollPanel();
  bool openCamera();
  void closeCamera();
  bool openMic();
  void closeMic();
  bool beginVideo(ESPVideoCSIConfigClass& csi);
  bool encodeStream(const uint8_t* src, int srcW, int srcH);
  bool encodeJpeg(const uint8_t* rgb, size_t rgbSize, uint16_t width, uint16_t height);
  bool encodeHw(const uint8_t* rgb, size_t rgbSize, uint16_t width, uint16_t height);
  bool probeSensor();
  void scaleRgb565(const uint8_t* src, int srcW, int srcH, uint16_t* dst, int dstW, int dstH);
  int readMicPeak();

  bool nullVfsReleased_ = false;
  i2c_master_bus_handle_t bus_ = nullptr;
  auth::IAuthSessionService* auth_ = nullptr;
  P4MicPump micPump_;
  P4Es8311 codec_;
  volatile bool wantsBlank_ = false;
  volatile bool panelRequested_ = false;
  volatile P4LiveClaim claim_ = P4LiveClaim::None;
  bool cameraOpen_ = false;
  bool micOpen_ = false;
  bool jpegStarted_ = false;
  bool micStarted_ = false;
  bool panelFrameReady_ = false;
  bool hwUnsupported_ = false;
  bool hwFrameFailed_ = false;
  char jpegOrigin_[96] = {};
  char micOrigin_[96] = {};
  char cameraFail_[32] = "camera_unavailable";
  char probeNote_[64] = "probe=idle";
  volatile uint32_t panelFrames_ = 0;
  uint32_t nextOpenMs_ = 0;
  uint8_t* jpegOut_ = nullptr;
  size_t jpegOutCapacity_ = 0;
  size_t jpegOutSize_ = 0;
  uint16_t* panelRgb_ = nullptr;
  uint16_t* streamRgb_ = nullptr;
  int panelW_ = 0;
  int panelH_ = 0;
  int panelPeak_ = 0;
  portMUX_TYPE panelMux_ = portMUX_INITIALIZER_UNLOCKED;
  WiFiServer jpegServer{kJpegPort};
  WiFiServer micServer{kMicPort};
  WiFiClient jpegClient;
  WiFiClient micClient;
  ESPVideoClass video;
  ESPVideoCaptureDevClass capture;
  I2SClass microphone{I2S_NUM_0};
  jpeg_encoder_handle_t hwJpeg_ = nullptr;
};

}  // namespace wattcycle::panel
