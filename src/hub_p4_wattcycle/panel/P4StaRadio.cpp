#include "P4StaRadio.h"

#include "Config/TimingConstants.h"
#include "Display/P4LiveBoard.h"
#include "Display/P4MacText.h"
#include "EspNow/EspNowPacketApply.h"
#include "P4BuildSecrets.h"
#include "Util/MacAddress.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_hosted_misc.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include <cstdio>
#include <cstring>

namespace wattcycle::panel {
namespace {

constexpr uint32_t kJoinTimeoutMs = 20000;

P4StaRadio* radioInstance = nullptr;

const char* wifiNote(bool wifiUp, bool joining, const char* ip) {
  if (wifiUp && ip != nullptr && ip[0] != '\0') {
    return "up";
  }
  if (wifiUp) {
    return "no address";
  }
  if (joining) {
    return "joining";
  }
  return "down";
}

bool macIsZero(const uint8_t mac[6]) {
  return mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0;
}

bool readC6StaMac(uint8_t mac[6], const char** source) {
  if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK && !macIsZero(mac)) {
    *source = "wifi-sta";
    return true;
  }
  if (esp_hosted_iface_mac_addr_get(mac, 6, ESP_MAC_WIFI_STA) == ESP_OK && !macIsZero(mac)) {
    *source = "hosted-sta";
    return true;
  }
  if (esp_hosted_iface_mac_addr_get(mac, 6, ESP_MAC_BASE) == ESP_OK && !macIsZero(mac)) {
    *source = "hosted-base";
    return true;
  }
  return false;
}

bool storeC6Mac(display::P4PanelBoard& board) {
  uint8_t mac[6] = {};
  const char* source = nullptr;
  if (!readC6StaMac(mac, &source)) {
    return false;
  }
  char text[display::kP4MacText] = {};
  std::snprintf(text, sizeof(text), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3],
                mac[4], mac[5]);
  display::copyMacText(board.gateway.hubMac, text);
  display::copyMacText(board.esp.deviceMac, text);
  Serial.printf("c6 sta mac %s via %s\n", text, source);
  return true;
}

bool addPeer(const uint8_t mac[6], bool encrypt, const uint8_t* pmk) {
  if (esp_now_is_peer_exist(mac)) {
    return true;
  }
  esp_now_peer_info_t peer = {};
  std::memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = encrypt;
  if (encrypt && pmk != nullptr) {
    std::memcpy(peer.lmk, pmk, 16);
  }
  return esp_now_add_peer(&peer) == ESP_OK;
}

void onEspNowFrame(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (info == nullptr || info->src_addr == nullptr || radioInstance == nullptr) {
    return;
  }
  radioInstance->onFrame(info->src_addr, data, len);
}

}  // namespace

void P4StaRadio::onFrame(const uint8_t* mac, const uint8_t* data, int len) {
  espnow_rx::ingestEspNowFrame(mac, data, len, xtPeerMac_, hasXtPeer_, pendingSolar_, pendingEco_,
                               pendingBms_, pendingProduct_, solarPacket_, ecoPacket_, bmsPacket_,
                               productPacket_);
}

void P4StaRadio::begin(display::P4PanelBoard& board, telemetry::ITelemetryStore& store) {
  radioInstance = this;
  store_ = &store;
  display::copyMacText(board.gateway.bmsMac, P4_BMS_MAC);
  display::copyMacText(board.gateway.ecoMac, P4_ECO_MAC);
  display::copyMacText(board.gateway.solarMac, P4_SOLAR_MAC);
  display::copyMacText(board.gateway.hubMac, "");
  display::copyMacText(board.esp.deviceMac, "");
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  haveMac_ = storeC6Mac(board);
  if (!haveMac_) {
    Serial.println("c6 sta mac pending");
  }
  if (P4_WIFI_SSID[0] == '\0') {
    Serial.println("wifi ssid missing");
    return;
  }
  WiFi.begin(P4_WIFI_SSID, P4_WIFI_PASS);
  joinStarted_ = true;
  joinStartedMs_ = millis();
  xTaskCreatePinnedToCore(&P4StaRadio::serviceTask, "p4radio", 16384, this, 1, nullptr, 0);
  Serial.printf("wifi joining ssidLen=%u passLen=%u\n",
                static_cast<unsigned>(std::strlen(P4_WIFI_SSID)),
                static_cast<unsigned>(std::strlen(P4_WIFI_PASS)));
}

bool P4StaRadio::startEspNow() {
  encrypt_ = std::strlen(P4_ESPNOW_PMK) >= 16;
  const uint8_t* pmk = reinterpret_cast<const uint8_t*>(P4_ESPNOW_PMK);
  if (esp_now_register_recv_cb(onEspNowFrame) != ESP_OK) {
    return false;
  }
  const esp_err_t init = esp_now_init();
  if (init != ESP_OK) {
    Serial.printf("esp-now init status=%d\n", static_cast<int>(init));
    return false;
  }
  if (encrypt_ && esp_now_set_pmk(pmk) != ESP_OK) {
    Serial.println("esp-now pmk failed");
    return false;
  }
  uint8_t bms[6] = {};
  uint8_t eco[6] = {};
  uint8_t solar[6] = {};
  const bool haveBms = util::parseMacAddress(P4_BMS_MAC, bms);
  const bool haveEco = util::parseMacAddress(P4_ECO_MAC, eco);
  const bool haveSolar = util::parseMacAddress(P4_SOLAR_MAC, solar);
  if (haveBms) {
    addPeer(bms, encrypt_, pmk);
  }
  if (haveEco) {
    addPeer(eco, encrypt_, pmk);
  }
  if (haveSolar) {
    addPeer(solar, encrypt_, pmk);
    std::memcpy(xtPeerMac_, solar, 6);
    hasXtPeer_ = true;
  }
  Serial.printf("esp-now rx ready enc=%u ch=%d\n", encrypt_ ? 1u : 0u, WiFi.channel());
  return true;
}

bool P4StaRadio::applyPending() {
  bool dirty = false;
  if (pendingSolar_) {
    pendingSolar_ = false;
    espnow_rx::applySolarPacket((*store_), encrypt_, solarPacket_);
    dirty = true;
  }
  if (pendingEco_) {
    pendingEco_ = false;
    espnow_rx::applyEcoFlowPacket((*store_), encrypt_, ecoPacket_);
    dirty = true;
  }
  if (pendingBms_) {
    pendingBms_ = false;
    espnow_rx::applyBmsTelemetryPacket((*store_), bmsPacket_);
    dirty = true;
  }
  if (pendingProduct_) {
    pendingProduct_ = false;
    espnow_rx::applyBmsProductPacket((*store_), productPacket_);
    dirty = true;
  }
  return dirty;
}

bool P4StaRadio::expireLinks() {
  using wattcycle::config::TimingConstants;
  bool dirty = false;
  const uint32_t now = millis();
  auto solar = (*store_).solar();
  if (solar.receivedAtMs != 0 && (now - solar.receivedAtMs) > TimingConstants::kSolarLinkStaleMs &&
      solar.linkFresh) {
    solar.linkFresh = false;
    solar.sppConnected = false;
    (*store_).updateSolar(solar);
    dirty = true;
  }
  auto eco = (*store_).ecoflow();
  if (eco.receivedAtMs != 0 && (now - eco.receivedAtMs) > TimingConstants::kEcoFlowLinkStaleMs &&
      eco.linkFresh) {
    eco.linkFresh = false;
    eco.bleConnected = false;
    (*store_).updateEcoFlow(eco);
    dirty = true;
  }
  const auto battery = (*store_).battery();
  if (battery.updatedAtMs != 0 && (now - battery.updatedAtMs) > TimingConstants::kBmsTelemetryStaleMs) {
    const auto status = (*store_).status();
    if (status.telemetryFresh || status.bleConnected) {
      (*store_).setBmsLink(false, false);
      dirty = true;
    }
  }
  return dirty;
}

bool P4StaRadio::rememberStaIp(display::P4PanelBoard& board) {
  char next[16] = {};
  if (board.gateway.wifiUp) {
    const IPAddress ip = WiFi.localIP();
    if (ip[0] != 0 || ip[1] != 0 || ip[2] != 0 || ip[3] != 0) {
      std::snprintf(next, sizeof(next), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    }
  }
  if (std::strcmp(board.esp.staIp, next) == 0) {
    return false;
  }
  std::snprintf(board.esp.staIp, sizeof(board.esp.staIp), "%s", next);
  return true;
}

void P4StaRadio::pollC6Heap() {
  if (!wifiUpFlag_ || millis() < nextC6HeapMs_) {
    return;
  }
  nextC6HeapMs_ = millis() + 5000;
  esp_hosted_config_mem_monitor_t config = {};
  config.config = ESP_HOSTED_MEMMONITOR_NO_CHANGE;
  esp_hosted_curr_mem_info_t info = {};
  if (esp_hosted_set_mem_monitor(&config, &info) != ESP_OK) {
    return;
  }
  const uint32_t freeBytes = info.curr_internal.cap_8bit.free_size;
  if (freeBytes == 0 && info.curr_total_heap_size == 0) {
    return;
  }
  const uint32_t shown = freeBytes != 0 ? freeBytes : info.curr_total_heap_size;
  c6HeapKb_ = static_cast<int>(shown / 1024u);
}

void P4StaRadio::serviceRadio() {
  if (!wifiUpFlag_ || store_ == nullptr) {
    return;
  }
  pollC6Heap();
  if (!espNowReady_ && millis() >= nextEspNowTryMs_) {
    nextEspNowTryMs_ = millis() + 5000;
    if (startEspNow()) {
      espNowReady_ = true;
    } else if (!c6UpdateStarted_) {
      c6UpdateStarted_ = true;
      c6_.start();
    }
  }
  if (applyPending() || expireLinks()) {
    ++revision_;
  }
}

void P4StaRadio::serviceTask(void* arg) {
  auto* self = static_cast<P4StaRadio*>(arg);
  for (;;) {
    self->serviceRadio();
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

bool P4StaRadio::loop(display::P4PanelBoard& board) {
  bool dirty = false;
  if (!haveMac_ && storeC6Mac(board)) {
    haveMac_ = true;
    dirty = true;
  }
  const bool wifiUp = WiFi.status() == WL_CONNECTED;
  wifiUpFlag_ = wifiUp;
  const bool wifiChanged = wifiUp != board.gateway.wifiUp;
  if (wifiChanged) {
    board.gateway.wifiUp = wifiUp;
    dirty = true;
  }
  if (joinStarted_ && wifiUp) {
    Serial.printf("wifi joined ch=%d\n", WiFi.channel());
    joinStarted_ = false;
  } else if (joinStarted_ && !wifiUp && (millis() - joinStartedMs_) > kJoinTimeoutMs) {
    Serial.printf("wifi timeout status=%d\n", static_cast<int>(WiFi.status()));
    WiFi.disconnect(false, false);
    WiFi.begin(P4_WIFI_SSID, P4_WIFI_PASS);
    joinStartedMs_ = millis();
  }
  const bool ipChanged = rememberStaIp(board);
  if (ipChanged) {
    dirty = true;
  }
  if (store_ != nullptr && (wifiChanged || ipChanged)) {
    store_->setWifiState(wifiUp, board.esp.staIp);
  }
  const char* note = wifiNote(wifiUp, joinStarted_, board.esp.staIp);
  if (std::strcmp(board.gateway.wifiNote, note) != 0) {
    std::snprintf(board.gateway.wifiNote, sizeof(board.gateway.wifiNote), "%s", note);
    dirty = true;
  }
  const int c6HeapKb = c6HeapKb_;
  if (c6HeapKb != board.esp.c6HeapKb) {
    board.esp.c6HeapKb = c6HeapKb;
    if (store_ != nullptr) {
      store_->setCoprocessorHeapKb(c6HeapKb);
    }
    dirty = true;
  }
  const uint32_t seen = revision_;
  if (seen != paintedRevision_) {
    paintedRevision_ = seen;
    display::fillP4LiveBoard(board, (*store_));
    dirty = true;
  }
  if (!haveMac_ && wifiUp && !toldJoinWithoutMac_) {
    Serial.println("c6 sta mac still zero after join");
    toldJoinWithoutMac_ = true;
  }
  return dirty;
}

}  // namespace wattcycle::panel
