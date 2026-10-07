#include "Wifi/EspWifiConnector.h"

#ifndef UNIT_TEST

#include <WiFi.h>
#include <cstring>

namespace wattcycle::wifi {

bool EspWifiConnector::connect(const char* ssid, const char* password,
                               uint32_t timeoutMs) {
  if (ssid == nullptr || password == nullptr || ssid[0] == '\0') {
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startedAt > timeoutMs) {
      return false;
    }
    delay(200);
  }
  return true;
}

bool EspWifiConnector::isConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

void EspWifiConnector::copyIpAddress(char* buffer, size_t capacity) const {
  if (buffer == nullptr || capacity == 0) {
    return;
  }
  const String ip = WiFi.localIP().toString();
  std::strncpy(buffer, ip.c_str(), capacity - 1);
  buffer[capacity - 1] = '\0';
}

void EspWifiConnector::loop() {
  // STA reconnect is handled by the ESP32 WiFi stack.
}

}  // namespace wattcycle::wifi

#endif
