#pragma once

#include <esp_now.h>

namespace wattcycle::panel {

/// Open the coprocessor ESP-NOW endpoint.
esp_err_t hostedNowInit();
/// Install the 16-byte primary key.
esp_err_t hostedNowSetPmk(const uint8_t* pmk);
/// Add a peer. An existing peer counts as success.
esp_err_t hostedNowAddPeer(const esp_now_peer_info_t* peer);
/// Send one frame to a peer that was already added.
esp_err_t hostedNowSend(const uint8_t* peer, const uint8_t* data, size_t len);
/// Receive callback for frames the coprocessor forwards.
esp_err_t hostedNowOnRecv(esp_now_recv_cb_t callback);

}  // namespace wattcycle::panel
