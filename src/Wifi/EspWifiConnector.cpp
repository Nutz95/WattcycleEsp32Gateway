#include "Wifi/EspWifiConnector.h"

#ifndef UNIT_TEST

#include "Util/SafeCopy.h"

#include <WiFi.h>
#include <cstring>

namespace wattcycle::wifi {

bool EspWifiConnector::connect(const char* ssid, const char* password,
                               uint32_t timeoutMs) {
  if (ssid == nullptr || password == nullptr || ssid[0] == '\0') {
    return false;
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  // Keep modem sleep on: classic ESP32 Wi-Fi+BLE coex aborts in
  // coex_core_enable if sleep is forced off before NimBLE init.
  Serial.printf("This board STA MAC: %s\n", WiFi.macAddress().c_str());
  Serial.println(F("  -> set as ESPNOW_PEER_MAC on the XT369P bridge"));
  WiFi.begin(ssid, password);

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startedAt > timeoutMs) {
      Serial.println(F("Wi-Fi connect timeout - auto-reconnect left enabled"));
      return false;
    }
    delay(200);
  }
  Serial.printf("Wi-Fi OK %s heap=%u\n", WiFi.localIP().toString().c_str(),
                ESP.getFreeHeap());
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

void EspWifiConnector::loop() {
  // STA reconnect is handled by the ESP32 WiFi stack.
}

}  // namespace wattcycle::wifi

#endif
