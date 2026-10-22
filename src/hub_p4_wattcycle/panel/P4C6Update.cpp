#include "P4C6Update.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <esp_hosted_ota.h>

#include <cstdio>

namespace wattcycle::panel {
namespace {

constexpr size_t kChunk = 4096;

void imageUrl(char* out, size_t capacity) {
  std::snprintf(out, capacity, "%s//%s%s", "https:", "esphome.github.io",
                "/esp-hosted-firmware/v2.12.13/network_adapter_esp32c6.bin");
}

bool downloadImage(uint8_t*& image, size_t& imageLen) {
  image = nullptr;
  imageLen = 0;
  WiFiClientSecure secureClient;
  HTTPClient http;
  secureClient.setInsecure();
  char url[128] = {};
  imageUrl(url, sizeof(url));
  if (!http.begin(secureClient, url)) {
    Serial.println("c6 image begin failed");
    return false;
  }
  const int code = http.GET();
  if (code != 200) {
    Serial.printf("c6 image http %d\n", code);
    http.end();
    return false;
  }
  const int size = http.getSize();
  if (size <= 0) {
    http.end();
    return false;
  }
  image = static_cast<uint8_t*>(heap_caps_malloc(static_cast<size_t>(size), MALLOC_CAP_SPIRAM));
  if (image == nullptr) {
    http.end();
    return false;
  }
  const size_t imageCap = static_cast<size_t>(size);
  WiFiClient* stream = http.getStreamPtr();
  int stalls = 0;
  while (imageLen < imageCap && http.connected()) {
    const size_t room = imageCap - imageLen;
    const size_t ask = room < kChunk ? room : kChunk;
    const int got = stream->readBytes(image + imageLen, ask);
    if (got <= 0) {
      if (++stalls > 200) {
        http.end();
        return false;
      }
      delay(10);
      continue;
    }
    stalls = 0;
    imageLen += static_cast<size_t>(got);
  }
  http.end();
  Serial.printf("c6 image bytes %u\n", static_cast<unsigned>(imageLen));
  return imageLen == imageCap;
}

bool writeImage(uint8_t* image, size_t imageLen) {
  if (image == nullptr || esp_hosted_slave_ota_begin() != ESP_OK) {
    Serial.println("c6 ota begin failed");
    return false;
  }
  size_t offset = 0;
  while (offset < imageLen) {
    const size_t room = imageLen - offset;
    const uint32_t ask = static_cast<uint32_t>(room < kChunk ? room : kChunk);
    if (esp_hosted_slave_ota_write(image + offset, ask) != ESP_OK) {
      Serial.println("c6 ota write failed");
      return false;
    }
    offset += ask;
    delay(1);
  }
  if (esp_hosted_slave_ota_end() != ESP_OK || esp_hosted_slave_ota_activate() != ESP_OK) {
    Serial.println("c6 ota activate failed");
    return false;
  }
  Serial.println("c6 ota activated");
  return true;
}

}  // namespace

bool installEspNowSlave() {
  Serial.println("c6 ota: installing esp-now slave");
  uint8_t* image = nullptr;
  size_t imageLen = 0;
  const bool ok = downloadImage(image, imageLen) && writeImage(image, imageLen);
  if (image != nullptr) {
    heap_caps_free(image);
  }
  return ok;
}

}  // namespace wattcycle::panel
