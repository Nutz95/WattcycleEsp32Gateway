#include "P4WebPublish.h"

#include <LittleFS.h>
#include <SD_MMC.h>

#include <Arduino.h>
#include <cstring>

namespace wattcycle::panel {
namespace {

bool sameStamp() {
  File left = LittleFS.open("/bundle.txt", "r");
  File right = SD_MMC.open("/bundle.txt", "r");
  if (!left || !right) {
    if (left) {
      left.close();
    }
    if (right) {
      right.close();
    }
    return false;
  }
  char a[80] = {};
  char b[80] = {};
  const int na = left.readBytes(a, sizeof(a) - 1);
  const int nb = right.readBytes(b, sizeof(b) - 1);
  left.close();
  right.close();
  return na > 0 && na == nb && std::memcmp(a, b, static_cast<size_t>(na)) == 0;
}

bool copyOne(const char* path) {
  File in = LittleFS.open(path, "r");
  if (!in) {
    return false;
  }
  SD_MMC.remove(path);
  File out = SD_MMC.open(path, "w");
  if (!out) {
    in.close();
    return false;
  }
  uint8_t buf[512];
  while (in.available()) {
    const int n = in.read(buf, sizeof(buf));
    if (n <= 0 || out.write(buf, static_cast<size_t>(n)) != static_cast<size_t>(n)) {
      in.close();
      out.close();
      return false;
    }
  }
  in.close();
  out.close();
  return true;
}

bool copySite() {
  static const char* paths[] = {"/index.html", "/app.css", "/app.js", "/bundle.txt"};
  for (const char* path : paths) {
    if (!copyOne(path)) {
      Serial.printf("web sd: copy failed %s\n", path);
      return false;
    }
  }
  return true;
}

}  // namespace

bool P4WebPublish::install() {
  if (SD_MMC.cardType() == CARD_NONE) {
    return false;
  }
  if (!LittleFS.begin(false)) {
    return SD_MMC.exists("/index.html");
  }
  if (!sameStamp() && !copySite()) {
    Serial.println("web sd: keeping LittleFS");
    return false;
  }
  LittleFS.end();
  const bool ready = SD_MMC.exists("/index.html");
  Serial.println(ready ? "web sd: serving from SD" : "web sd: index missing");
  return ready;
}

}  // namespace wattcycle::panel
