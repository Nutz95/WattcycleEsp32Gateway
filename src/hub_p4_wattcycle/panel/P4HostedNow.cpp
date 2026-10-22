#include "P4HostedNow.h"

#include <esp_now.h>
#include <esp_wifi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <cstring>

extern "C" {
#include "esp_hosted_misc.h"
}

namespace {
constexpr uint32_t kReqMsg = 0x6E6F7701u;
constexpr uint32_t kRespMsg = 0x6E6F7702u;
constexpr uint32_t kRecvMsg = 0x6E6F7703u;
constexpr uint8_t kOpInit = 1;
constexpr uint8_t kOpAddPeer = 3;
constexpr uint8_t kOpSend = 5;
constexpr uint8_t kOpSetPmk = 8;
constexpr uint32_t kWaitMs = 2000;
constexpr size_t kPeerSlots = 20;

SemaphoreHandle_t reqMutex = nullptr;
SemaphoreHandle_t respSem = nullptr;
uint8_t nextSeq = 0;
volatile uint8_t expectSeq = 0;
volatile int32_t respStatus = ESP_FAIL;
esp_now_recv_cb_t recvCallback = nullptr;
uint8_t peerCache[kPeerSlots][6] = {};
size_t peerCount = 0;
bool handlersReady = false;

bool peerCached(const uint8_t mac[6]) {
  for (size_t i = 0; i < peerCount; ++i) {
    if (std::memcmp(peerCache[i], mac, 6) == 0) {
      return true;
    }
  }
  return false;
}

void rememberPeer(const uint8_t mac[6]) {
  if (!peerCached(mac) && peerCount < kPeerSlots) {
    std::memcpy(peerCache[peerCount], mac, 6);
    peerCount += 1;
  }
}

void onHostedResp(uint32_t msgId, const uint8_t* data, size_t len, void* context) {
  (void)msgId;
  (void)context;
  if (data == nullptr || len < 8 || data[1] != expectSeq) {
    return;
  }
  int32_t status = 0;
  std::memcpy(&status, data + 2, sizeof(status));
  respStatus = status;
  xSemaphoreGive(respSem);
}

void onHostedRecv(uint32_t msgId, const uint8_t* data, size_t len, void* context) {
  (void)msgId;
  (void)context;
  if (recvCallback == nullptr || data == nullptr || len < 16) {
    return;
  }
  uint16_t dataLen = 0;
  std::memcpy(&dataLen, data + 14, sizeof(dataLen));
  if (len < static_cast<size_t>(16 + dataLen)) {
    return;
  }
  uint8_t src[6] = {};
  uint8_t des[6] = {};
  std::memcpy(src, data, 6);
  std::memcpy(des, data + 6, 6);
  wifi_pkt_rx_ctrl_t ctrl = {};
  esp_now_recv_info_t info = {};
  info.src_addr = src;
  info.des_addr = des;
  info.rx_ctrl = &ctrl;
  recvCallback(&info, data + 16, static_cast<int>(dataLen));
}

bool installHandlers() {
  if (handlersReady) {
    return true;
  }
  reqMutex = xSemaphoreCreateMutex();
  respSem = xSemaphoreCreateBinary();
  if (reqMutex == nullptr || respSem == nullptr) {
    return false;
  }
  const esp_err_t resp = esp_hosted_register_custom_callback(kRespMsg, onHostedResp, nullptr);
  const esp_err_t recv = esp_hosted_register_custom_callback(kRecvMsg, onHostedRecv, nullptr);
  handlersReady = resp == ESP_OK && recv == ESP_OK;
  return handlersReady;
}

esp_err_t transact(uint8_t opcode, const uint8_t* payload, uint16_t payloadLen) {
  if (!installHandlers()) {
    return ESP_FAIL;
  }
  if (xSemaphoreTake(reqMutex, pdMS_TO_TICKS(kWaitMs)) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }
  uint8_t frame[4 + 32] = {};
  if (payloadLen > 32) {
    xSemaphoreGive(reqMutex);
    return ESP_ERR_INVALID_SIZE;
  }
  frame[0] = opcode;
  frame[1] = ++nextSeq;
  frame[2] = static_cast<uint8_t>(payloadLen & 0xFF);
  frame[3] = static_cast<uint8_t>((payloadLen >> 8) & 0xFF);
  if (payload != nullptr && payloadLen > 0) {
    std::memcpy(frame + 4, payload, payloadLen);
  }
  expectSeq = frame[1];
  xSemaphoreTake(respSem, 0);
  const esp_err_t sent = esp_hosted_send_custom_data(kReqMsg, frame, 4u + payloadLen);
  if (sent != ESP_OK) {
    xSemaphoreGive(reqMutex);
    return sent;
  }
  if (xSemaphoreTake(respSem, pdMS_TO_TICKS(kWaitMs)) != pdTRUE) {
    xSemaphoreGive(reqMutex);
    return ESP_ERR_TIMEOUT;
  }
  const esp_err_t status = respStatus;
  xSemaphoreGive(reqMutex);
  return status;
}

}  // namespace

namespace wattcycle::panel {

esp_err_t hostedNowInit() {
  return transact(kOpInit, nullptr, 0);
}

esp_err_t hostedNowSetPmk(const uint8_t* pmk) {
  if (pmk == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  return transact(kOpSetPmk, pmk, 16);
}

esp_err_t hostedNowAddPeer(const esp_now_peer_info_t* peer) {
  if (peer == nullptr) {
    return ESP_ERR_INVALID_ARG;
  }
  uint8_t raw[25] = {};
  std::memcpy(raw, peer->peer_addr, 6);
  if (peer->encrypt) {
    std::memcpy(raw + 6, peer->lmk, 16);
  }
  raw[22] = peer->channel;
  raw[23] = static_cast<uint8_t>(peer->ifidx);
  raw[24] = peer->encrypt ? 1 : 0;
  const esp_err_t status = transact(kOpAddPeer, raw, sizeof(raw));
  if (status == ESP_OK || status == ESP_ERR_ESPNOW_EXIST) {
    rememberPeer(peer->peer_addr);
    return ESP_OK;
  }
  return status;
}

esp_err_t hostedNowSend(const uint8_t* peer, const uint8_t* data, size_t len) {
  if (peer == nullptr || data == nullptr || len == 0 || len > 26) {
    return ESP_ERR_INVALID_ARG;
  }
  uint8_t raw[32] = {};
  std::memcpy(raw, peer, 6);
  std::memcpy(raw + 6, data, len);
  return transact(kOpSend, raw, static_cast<uint16_t>(6u + len));
}

esp_err_t hostedNowOnRecv(esp_now_recv_cb_t cb) {
  recvCallback = cb;
  return ESP_OK;
}

}  // namespace wattcycle::panel
