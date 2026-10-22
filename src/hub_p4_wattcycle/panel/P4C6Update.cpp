#include "P4C6Update.h"

#include <Arduino.h>

#include <cstdio>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>

#include <esp_hosted_ota.h>

namespace wattcycle::panel {
namespace {

constexpr size_t kChunk = 4096;

void imageUrl(char* out, size_t capacity) {
  std::snprintf(out, capacity, "%s//%s%s", "https:", "esphome.github.io",
                "/esp-hosted-firmware/v2.12.13/network_adapter_esp32c6.bin");
}

WiFiClientSecure secureClient;
HTTPClient http;
uint8_t* image = nullptr;
size_t imageLen = 0;
size_t imageCap = 0;

bool downloadImage() {
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
  imageCap = static_cast<size_t>(size);
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

bool writeImage() {
  if (esp_hosted_slave_ota_begin() != ESP_OK) {
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

void P4C6Update::start() {
  if (started_) {
    return;
  }
  started_ = true;
  Serial.println("c6 ota: installing esp-now slave");
  failed_ = !downloadImage() || !writeImage();
  finished_ = !failed_;
}

bool P4C6Update::loop() {
  return finished_;
}

bool P4C6Update::failed() const {
  return failed_;
}

}  // namespace wattcycle::panel
