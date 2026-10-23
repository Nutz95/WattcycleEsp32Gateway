#include "P4LiveView.h"

#include "P4LiveHttp.h"

#include <Arduino.h>
#include <WiFi.h>

#include <cstdio>
#include <cstring>

namespace wattcycle::panel {

void P4LiveView::begin(i2c_master_bus_handle_t bus, auth::IAuthSessionService& auth) {
  bus_ = bus;
  auth_ = &auth;
  micPump_.start(microphone);
  xTaskCreatePinnedToCore(&P4LiveView::taskMain, "liveView", 16384, this, 1, nullptr, 1);
}

bool P4LiveView::wantsBlank() const {
  return wantsBlank_;
}

void P4LiveView::requestPanel(bool want) {
  panelRequested_ = want;
}

void P4LiveView::applyRequest() {
  if (panelRequested_) {
    if (claim_ == P4LiveClaim::None) {
      claim_ = P4LiveClaim::Panel;
      wantsBlank_ = false;
    }
    return;
  }
  if (claim_ == P4LiveClaim::Panel) {
    claim_ = P4LiveClaim::None;
    panelFrameReady_ = false;
    closeCamera();
    closeMic();
  }
}

void P4LiveView::rejectBusy(WiFiServer& server, const char* reason) {
  WiFiClient incoming = server.accept();
  if (!incoming) {
    return;
  }
  liveSendStatus(incoming, 503, reason);
  incoming.stop();
}

bool P4LiveView::panelClaimed() const {
  return claim_ == P4LiveClaim::Panel;
}

const char* P4LiveView::cameraFail() const {
  return cameraFail_;
}

bool P4LiveView::pullPanelFrame(uint16_t* rgb565, int outWidth, int outHeight, int& micPeak) {
  if (rgb565 == nullptr || outWidth <= 0 || outHeight <= 0) {
    return false;
  }
  portENTER_CRITICAL(&panelMux_);
  const bool ready = panelFrameReady_ && panelRgb_ != nullptr && panelW_ == outWidth && panelH_ == outHeight;
  if (ready) {
    std::memcpy(rgb565, panelRgb_, static_cast<size_t>(outWidth * outHeight) * sizeof(uint16_t));
    micPeak = panelPeak_;
  }
  portEXIT_CRITICAL(&panelMux_);
  return ready;
}

void P4LiveView::taskMain(void* context) {
  static_cast<P4LiveView*>(context)->run();
}

void P4LiveView::dropServers() {
  jpegClient.stop();
  micClient.stop();
  jpegServer.end();
  micServer.end();
  jpegStarted_ = false;
  micStarted_ = false;
  wantsBlank_ = false;
  if (claim_ == P4LiveClaim::Web) {
    claim_ = P4LiveClaim::None;
    closeCamera();
    closeMic();
  }
}

bool P4LiveView::ensureServers() {
  if (jpegServer && micServer) {
    return true;
  }
  jpegServer.end();
  micServer.end();
  jpegServer.begin(kJpegPort);
  micServer.begin(kMicPort);
  jpegServer.setNoDelay(true);
  micServer.setNoDelay(true);
  if (!jpegServer || !micServer) {
    jpegServer.end();
    micServer.end();
    return false;
  }
  Serial.printf("live streams listening :%u :%u\n", kJpegPort, kMicPort);
  return true;
}

void P4LiveView::run() {
  for (;;) {
    applyRequest();
    if (claim_ == P4LiveClaim::Panel) {
      pollPanel();
      if (WiFi.status() == WL_CONNECTED) {
        ensureServers();
        rejectBusy(jpegServer, "panel_busy");
        rejectBusy(micServer, "panel_busy");
      }
      delay(40);
      continue;
    }
    if (WiFi.status() != WL_CONNECTED || auth_ == nullptr) {
      dropServers();
      delay(200);
      continue;
    }
    if (!ensureServers()) {
      delay(500);
      continue;
    }
    pollJpeg();
    pollMic();
    delay(2);
  }
}

}  // namespace wattcycle::panel
