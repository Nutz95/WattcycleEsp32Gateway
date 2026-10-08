#include "Meter/Spp/AtorchSppClient.h"

#include "Meter/Protocol/AtorchCommands.h"
#include "Util/SafeCopy.h"

#include <cstdio>
#include <cstring>
#include <string>

#ifndef UNIT_TEST
#include <Arduino.h>
#include <BTAddress.h>
#include <BTAdvertisedDevice.h>
#include <BTScan.h>
#endif

namespace xt369p::meter {

bool AtorchSppClient::parseBtAddress(const char* text, uint8_t out[6]) {
  if (text == nullptr || text[0] == '\0') {
    return false;
  }
  unsigned int b[6] = {};
  if (std::sscanf(text, "%02x:%02x:%02x:%02x:%02x:%02x", &b[0], &b[1], &b[2], &b[3], &b[4],
                  &b[5]) != 6 &&
      std::sscanf(text, "%02X:%02X:%02X:%02X:%02X:%02X", &b[0], &b[1], &b[2], &b[3], &b[4],
                  &b[5]) != 6) {
    return false;
  }
  for (int i = 0; i < 6; ++i) {
    out[i] = static_cast<uint8_t>(b[i]);
  }
  return true;
}

bool AtorchSppClient::looksLikeBogusAddress(const uint8_t addr[6]) {
  // Windows sometimes exposes truncated / placeholder IDs like 00:00:00:03:12:AE.
  return addr[0] == 0 && addr[1] == 0 && addr[2] == 0;
}

bool AtorchSppClient::beginAsMaster(const char* localName) {
#ifndef UNIT_TEST
  if (started_) {
    return true;
  }
  const char* name = (localName && localName[0]) ? localName : "XT369P_GW";
  if (!serialBt_.begin(name, true)) {
    util::copyCString(lastError_, sizeof(lastError_), "bt_begin_failed");
    return false;
  }
  started_ = true;
  util::copyCString(lastError_, sizeof(lastError_), "");
  return true;
#else
  (void)localName;
  started_ = true;
  return true;
#endif
}

bool AtorchSppClient::connectAddress(const char* btAddress) {
#ifndef UNIT_TEST
  if (!started_ && !beginAsMaster("XT369P_GW")) {
    return false;
  }
  uint8_t addr[6] = {};
  if (!parseBtAddress(btAddress, addr)) {
    util::copyCString(lastError_, sizeof(lastError_), "bad_bt_address");
    connected_ = false;
    return false;
  }
  if (looksLikeBogusAddress(addr)) {
    util::copyCString(lastError_, sizeof(lastError_), "bogus_bt_address");
    connected_ = false;
    return false;
  }
  Serial.printf("SPP connect address %02X:%02X:%02X:%02X:%02X:%02X\n", addr[0], addr[1], addr[2],
                addr[3], addr[4], addr[5]);
  connected_ = serialBt_.connect(addr);
  if (!connected_) {
    util::copyCString(lastError_, sizeof(lastError_), "spp_connect_failed");
    return false;
  }
  std::snprintf(peerAddress_, sizeof(peerAddress_), "%02X:%02X:%02X:%02X:%02X:%02X", addr[0],
                addr[1], addr[2], addr[3], addr[4], addr[5]);
  parser_.reset();
  util::copyCString(lastError_, sizeof(lastError_), "");
  return true;
#else
  (void)btAddress;
  connected_ = true;
  return true;
#endif
}

bool AtorchSppClient::connectName(const char* remoteName) {
#ifndef UNIT_TEST
  if (!started_ && !beginAsMaster("XT369P_GW")) {
    return false;
  }
  const char* name = (remoteName && remoteName[0]) ? remoteName : "XT369P_SPP";
  Serial.printf("SPP connect by name '%s' (inquiry)\n", name);
  connected_ = serialBt_.connect(String(name));
  if (!connected_) {
    util::copyCString(lastError_, sizeof(lastError_), "spp_connect_name_failed");
    return false;
  }
  util::copyCString(peerAddress_, sizeof(peerAddress_), name);
  parser_.reset();
  util::copyCString(lastError_, sizeof(lastError_), "");
  return true;
#else
  (void)remoteName;
  connected_ = true;
  return true;
#endif
}

bool AtorchSppClient::discoverAndConnect(const char* remoteName, uint32_t timeoutMs) {
#ifndef UNIT_TEST
  if (!started_ && !beginAsMaster("XT369P_GW")) {
    return false;
  }
  const char* name = (remoteName && remoteName[0]) ? remoteName : "XT369P_SPP";
  const int timeout = timeoutMs < 3000 ? 3000 : static_cast<int>(timeoutMs);
  Serial.printf("BT inquiry %d ms for '%s'...\n", timeout, name);

  BTScanResults* results = serialBt_.discover(timeout);
  if (results == nullptr) {
    util::copyCString(lastError_, sizeof(lastError_), "bt_discover_failed");
    connected_ = false;
    return false;
  }

  const int count = results->getCount();
  Serial.printf("BT inquiry found %d device(s)\n", count);
  int matchIndex = -1;
  for (int i = 0; i < count; ++i) {
    BTAdvertisedDevice* device = results->getDevice(i);
    if (device == nullptr) {
      continue;
    }
    const std::string deviceName = device->haveName() ? device->getName() : std::string("(no-name)");
    const BTAddress address = device->getAddress();
    Serial.printf("  [%d] %s  %s\n", i, address.toString().c_str(), deviceName.c_str());
    if (matchIndex < 0 && device->haveName() && strcasecmp(deviceName.c_str(), name) == 0) {
      matchIndex = i;
    }
  }

  if (matchIndex < 0) {
    util::copyCString(lastError_, sizeof(lastError_), "spp_name_not_found");
    connected_ = false;
    return false;
  }

  BTAdvertisedDevice* match = results->getDevice(matchIndex);
  const BTAddress address = match->getAddress();
  util::copyCString(peerAddress_, sizeof(peerAddress_), address.toString().c_str());
  Serial.printf("SPP connecting to discovered %s (%s)\n", peerAddress_, name);
  connected_ = serialBt_.connect(address);
  if (!connected_) {
    // Fallback: library name resolver (second inquiry).
    connected_ = serialBt_.connect(String(name));
  }
  if (!connected_) {
    util::copyCString(lastError_, sizeof(lastError_), "spp_connect_failed");
    return false;
  }
  parser_.reset();
  util::copyCString(lastError_, sizeof(lastError_), "");
  return true;
#else
  (void)remoteName;
  (void)timeoutMs;
  return false;
#endif
}

void AtorchSppClient::disconnect() {
#ifndef UNIT_TEST
  if (connected_) {
    serialBt_.disconnect();
  }
#endif
  connected_ = false;
  parser_.reset();
}

bool AtorchSppClient::isConnected() const {
  return connected_;
}

bool AtorchSppClient::poll(WattmeterTelemetry& out) {
#ifndef UNIT_TEST
  if (!connected_ || !serialBt_.connected()) {
    connected_ = false;
    return false;
  }
  uint8_t chunk[128];
  bool produced = false;
  while (serialBt_.available() > 0) {
    const int n = serialBt_.readBytes(chunk, sizeof(chunk));
    if (n <= 0) {
      break;
    }
    if (parser_.feed(chunk, static_cast<size_t>(n), out)) {
      produced = true;
    }
  }
  return produced;
#else
  (void)out;
  return false;
#endif
}

bool AtorchSppClient::sendCommand(MeterCommand command) {
#ifndef UNIT_TEST
  if (!connected_ || !serialBt_.connected()) {
    connected_ = false;
    util::copyCString(lastError_, sizeof(lastError_), "spp_not_connected");
    return false;
  }
  uint8_t frame[AtorchCommands::kFrameLen] = {};
  if (!AtorchCommands::encode(command, frame)) {
    util::copyCString(lastError_, sizeof(lastError_), "bad_command");
    return false;
  }
  const size_t written = serialBt_.write(frame, AtorchCommands::kFrameLen);
  if (written != AtorchCommands::kFrameLen) {
    util::copyCString(lastError_, sizeof(lastError_), "spp_write_failed");
    return false;
  }
  util::copyCString(lastError_, sizeof(lastError_), "");
  return true;
#else
  (void)command;
  return false;
#endif
}

const char* AtorchSppClient::lastError() const {
  return lastError_;
}

const char* AtorchSppClient::peerAddress() const {
  return peerAddress_;
}

}  // namespace xt369p::meter
