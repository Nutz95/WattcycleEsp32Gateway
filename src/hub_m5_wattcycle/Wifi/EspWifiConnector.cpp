#include "Wifi/EspWifiConnector.h"

#ifndef UNIT_TEST

#include "Util/SafeCopy.h"

#include <WiFi.h>
#include <cstring>

namespace wattcycle::wifi {
namespace {
constexpr uint32_t kRetryIntervalMs = 15000;
}

uint8_t EspWifiConnector::scanAndCache(const char* ssid) {
  if (ssid == nullptr || ssid[0] == '\0') {
    return 0;
  }
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, false);
  delay(100);
  const int n = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  cachedChannel_ = 0;
  haveBssid_ = false;
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) != ssid) {
      continue;
    }
    cachedChannel_ = static_cast<uint8_t>(WiFi.channel(i));
    const uint8_t* bssid = WiFi.BSSID(i);
    if (bssid != nullptr) {
      std::memcpy(bssid_, bssid, 6);
      haveBssid_ = true;
    }
    Serial.printf("Wi-Fi scan hit SSID='%s' ch=%u rssi=%d\n", ssid, cachedChannel_,
                  WiFi.RSSI(i));
    break;
  }
  if (cachedChannel_ == 0) {
    Serial.printf("Wi-Fi scan: SSID '%s' not among %d AP(s)\n", ssid, n);
  }
  WiFi.scanDelete();
  return cachedChannel_;
}

bool EspWifiConnector::connect(const char* ssid, const char* password, uint32_t timeoutMs) {
  if (ssid == nullptr || password == nullptr || ssid[0] == '\0') {
    return false;
  }

  wattcycle::util::copyCString(ssid_, sizeof(ssid_), ssid);
  wattcycle::util::copyCString(password_, sizeof(password_), password);

  // Match TTGO hub: avoid erase-style disconnect that left M5 STA at status=6.
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  Serial.printf("This board STA MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println(F("  -> set as ESPNOW_PEER_MAC on both TTGO bridges"));

  // Fingerprint only — confirms build-time password length/content without leaking it.
  unsigned sum = 0;
  for (const char* p = password; *p != '\0'; ++p) {
    sum = (sum * 131u) + static_cast<unsigned char>(*p);
  }
  Serial.printf("Wi-Fi cred fingerprint ssidLen=%u passLen=%u mix=%08x\n",
                static_cast<unsigned>(std::strlen(ssid)),
                static_cast<unsigned>(std::strlen(password)), sum);

  const uint8_t channel = scanAndCache(ssid);
  Serial.printf("Wi-Fi joining SSID='%s' ch=%u heap=%u\n", ssid, channel, ESP.getFreeHeap());
  if (haveBssid_ && channel >= 1) {
    WiFi.begin(ssid, password, channel, bssid_);
  } else if (channel >= 1) {
    WiFi.begin(ssid, password, channel);
  } else {
    WiFi.begin(ssid, password);
  }

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startedAt > timeoutMs) {
      Serial.printf("Wi-Fi connect timeout status=%d heap=%u\n",
                    static_cast<int>(WiFi.status()), ESP.getFreeHeap());
      lastRetryMs_ = millis();
      return false;
    }
    delay(200);
  }
  everConnected_ = true;
  cachedChannel_ = static_cast<uint8_t>(WiFi.channel());
  Serial.printf("Wi-Fi OK %s ch=%d heap=%u\n", WiFi.localIP().toString().c_str(),
                cachedChannel_, ESP.getFreeHeap());
  return true;
}

bool EspWifiConnector::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

void EspWifiConnector::copyIpAddress(char* buffer, size_t capacity) const {
  if (buffer == nullptr || capacity == 0) {
    return;
  }
  wattcycle::util::copyCString(buffer, capacity, WiFi.localIP().toString().c_str());
}

uint8_t EspWifiConnector::resolveSsidChannel(const char* ssid) {
  if (cachedChannel_ >= 1 && cachedChannel_ <= 13) {
    return cachedChannel_;
  }
  if (isConnected()) {
    cachedChannel_ = static_cast<uint8_t>(WiFi.channel());
    return cachedChannel_;
  }
  return scanAndCache(ssid != nullptr ? ssid : ssid_);
}

void EspWifiConnector::loop() {
  if (ssid_[0] == '\0' || WiFi.status() == WL_CONNECTED) {
    return;
  }
  // Never tear down the radio before the first successful IP join: ESP-NOW is
  // locked to the scanned AP channel and WiFi.begin/disconnect knocks it off.
  // Wrong passphrase (status=6 with strong RSSI) must not spam retries.
  if (!everConnected_) {
    return;
  }
  const uint32_t now = millis();
  if (lastRetryMs_ != 0 && (now - lastRetryMs_) < kRetryIntervalMs) {
    return;
  }
  lastRetryMs_ = now;
  Serial.printf("Wi-Fi reconnect status=%d heap=%u\n", static_cast<int>(WiFi.status()),
                ESP.getFreeHeap());
  WiFi.reconnect();
}

}  // namespace wattcycle::wifi

#endif
