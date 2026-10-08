# Wattcycle ESP32 Gateway

> Monorepo: **Wattcycle BMS hub** (BLE → Wi‑Fi web) + **XT369P solar bridge**
> (Classic SPP → ESP-NOW) on LILYGO TTGO T-Display boards — one dashboard on the LAN.

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32-orange)](https://platformio.org/)
[![Protocol](https://img.shields.io/badge/BLE-Wattcycle%20%2F%20XDZN-blue)](https://github.com/qume/wattcycle_ble)
[![Web](https://img.shields.io/badge/Web%20UI-port%206789-green)](#quick-start)

---

## Why this exists

Wattcycle packs expose rich BMS data over **Bluetooth**, but they do **not** ship a
routable web server. Phones work locally; remote / LAN dashboards do not.

Two firmwares, one repo:

```mermaid
flowchart LR
  BMS["Wattcycle BMS BLE"] --> Hub["hub_ttgo_wattcycle COM19"]
  XT["XT369P SPP"] --> Bridge["bridge_xt369p COM22"]
  Bridge -->|"ESP-NOW + PMK"| Hub
  Hub -->|"HTTP :6789 fused binary"| Browser
```

Prefer `.\scripts\pair_espnow_link.ps1` for first-time MAC + PMK setup, then
`.\scripts\flash.ps1 -Target Both`.

**No SD card required on the hub** — the dashboard (`data/`) is flashed into **LittleFS**.

### Roadmap (Phase 2)

Later: move the web hub to an **M5Stack Basic** (COM23, classic ESP32) and demote both
TTGOs to ESP-NOW transport-only bridges. Target shape:

```mermaid
flowchart TB
  subgraph bridges [Passerelles TTGO]
    BMS2["BMS BLE"] --> BridgeBms["bridge_ttgo_wattcycle COM19"]
    XT2["XT369P SPP"] --> BridgeXt["bridge_xt369p COM22"]
  end
  subgraph hub [Hub M5Stack Basic COM23]
    EspNowRx["ESP-NOW RX multi-peer"]
    Store["Telemetry store + SD history"]
    Web["HTTP SPA + auth"]
    Ntp["NTP clock"]
    PairUi["Web: scan / associate / NVS keys"]
  end
  BridgeBms -->|"ESP-NOW encrypted"| EspNowRx
  BridgeXt -->|"ESP-NOW encrypted"| EspNowRx
  EspNowRx --> Store --> Web
  PairUi --> EspNowRx
  Store --> SD[(SD card)]
  Ntp --> Store
```

---

## Hardware (probed)

| Item | Value |
|------|-------|
| Board | LILYGO TTGO T-Display |
| MCU | **ESP32-D0WDQ6** rev v1.0 *(classic ESP32 — not S3)* |
| Flash | 4 MB |
| Radio | Wi-Fi + Bluetooth (dual core @ 240 MHz) |
| Display | 1.14″ IPS **ST7789V** (135×240) |
| USB-UART | Silicon Labs CP210x |
| Probed port | **COM19** |
| MAC (example unit) | `24:6F:28:25:18:14` |

### ESP TTGO used on this project as the bluetooth-wifi gateway + display

<p align="center">
  <img src="Resources/ESP_TTGO.jpg" alt="LILYGO TTGO T-Display ESP32" width="720" />
</p>

### Display pinout

<p align="center">
  <img src="Resources/Esp32-TTGO_Pinout.png" alt="LILYGO TTGO T-Display ESP32 pinout" width="720" />
</p>

| Signal | GPIO |
|--------|------|
| MOSI | 19 |
| SCLK | 18 |
| CS | 5 |
| DC | 16 |
| RST | 23 |
| Backlight | 4 |

```mermaid
flowchart TB
  USB["USB-C / CP210x"] --> ESP["ESP32-D0WDQ6"]
  ESP --> TFT["ST7789V 1.14 inch"]
  ESP --> WIFI["Wi-Fi STA"]
  ESP --> BT["Bluetooth LE"]
  BT --> PACK["Wattcycle 51.2V pack"]
  WIFI --> UI["http://&lt;ip&gt;:6789"]
```

---

## Protocols

| Device | Doc |
|--------|-----|
| Wattcycle / XDZN BMS (BLE) | [docs/WATTCYCLE_PROTOCOL.md](docs/WATTCYCLE_PROTOCOL.md) · upstream [qume/wattcycle_ble](https://github.com/qume/wattcycle_ble) |
| ATorch XT369P (Classic SPP) | [docs/XT369P_PROTOCOL.md](docs/XT369P_PROTOCOL.md) |

### Wattcycle BLE (short)

| Step | Detail |
|------|--------|
| Discover | Names starting with `XDZN` or `WT` |
| Service | `0xFFF0` |
| Notify / Write / Auth | `FFF1` / `FFF2` / `FFFA` |
| Auth | Write ASCII `HiLink` to `FFFA` (no pairing) |
| Framing | Head `0x7E` (or `0x1E`) … Modbus CRC16 … tail `0x0D` |
| Main DP | Analog Quantity **140** (`0x8C`) — SoC, V, I, cells, temps… |

### XT369P solar meter

<p align="center">
  <img src="Resources/xt369p.png" alt="ATorch XT369P wattmeter" width="420" />
</p>

<p align="center">
  <img src="Resources/xt369p_front_back.png" alt="XT369P front and back" width="560" />
</p>

Classic SPP name `XT369P_SPP` → TTGO `bridge_xt369p` → ESP-NOW → hub. Frame layout and checksum notes are in the protocol doc above.

---

## Repository layout (monorepo)

```text
src/
  common/                 # Util + ESP-NOW protocols (+ shared Bms models)
  hub_common/             # Auth, HttpApi, Ota, Telemetry store, NTP helpers
  hub_m5_wattcycle/       # Phase 2 web hub (COM23): multi-peer ESP-NOW RX + SD
  bridge_ttgo_wattcycle/  # BMS BLE → ESP-NOW TX (COM19)
  bridge_xt369p/          # XT369P SPP → ESP-NOW TX (COM22)
  hub_ttgo_wattcycle/     # Legacy Phase 1 hub
web/                      # SPA (hub LittleFS only)
docs/                     # WATTCYCLE_PROTOCOL.md + XT369P_PROTOCOL.md
scripts/                  # flash.ps1 / pair_espnow_link / run_tests / …
test/                     # native Unity tests (hub parsers)
```

| PlatformIO env | Port | Role |
|----------------|------|------|
| `hub_m5_wattcycle` | COM23 | Web hub + multi-peer ESP-NOW RX + SD/NTP |
| `bridge_ttgo_wattcycle` | COM19 | BMS BLE → ESP-NOW TX |
| `bridge_xt369p` | COM22 | SPP → ESP-NOW TX (no web) |
| `hub_ttgo_wattcycle` | — | Legacy Phase 1 hub |
| `native` | — | Host unit tests |

Design goals: **SOLID**, constructor injection, files **&lt; 400 lines**, classes **&lt; 30 methods**, **no nested classes**.

---

## Quick Start

### 0. Prerequisites

- [PlatformIO Core](https://platformio.org/install/cli) (`pio` on PATH)
- PowerShell 5+ / 7+
- Hub TTGO on USB (**COM19**); optional XT369P bridge on **COM22**
- Environment variables (**required**, never commit secrets):

```powershell
$env:WIFI_SSID = "YourWifiName"
$env:WIFI_PASS = "YourWifiPassword"          # hub STA join (bridge uses SSID for channel only)
$env:BMS_BLE_ADDRESS = "AA:BB:CC:DD:EE:FF"   # BLE MAC only — no serial / password needed
# Optional solar bridge:
# $env:XT369P_BT_ADDRESS = "AA:BB:CC:DD:EE:FF"
# ESPNOW_* set by pair_espnow_link.ps1
```

> **BMS connection:** only the BLE MAC (`BMS_BLE_ADDRESS`) is required. Auth uses the fixed Wattcycle `HiLink` key over GATT (no pairing, no serial number). Find the MAC in the phone app or any BLE scanner (`XDZN…` / `WT…` names).

> Tip: leave `BMS_BLE_ADDRESS` unset only while bringing Wi-Fi/UI up; BLE polling will report a clear error on the display and `/api/telemetry.bin`.

### 1. Run unit tests + guardrails

```powershell
.\scripts\run_tests.ps1
```

### 2. Flash firmwares (USB)

```powershell
# Hub only (firmware + LittleFS)
.\scripts\flash.ps1 -Target HubTtgoWattcycle -Port COM19

# Bridge only
.\scripts\flash.ps1 -Target BridgeXt369p -Port COM22

# Both (distinct COM ports)
.\scripts\flash.ps1 -Target Both -HubPort COM19 -BridgePort COM22
```

`.\scripts\flash_usb.ps1` remains a thin alias for the hub.

### 2b. Pair XT369P solar bridge (ESP-NOW)

Plug **both** ESPs (hub COM19 + bridge COM22):

```powershell
.\scripts\pair_espnow_link.ps1 -HubPort COM19 -BridgePort COM22 -Flash
```

```text
BRIDGE (XT369P COM22)  --ESP-NOW-->  HUB (Wattcycle COM19)

ESPNOW_PEER_MAC   on BRIDGE = HUB MAC
ESPNOW_BRIDGE_MAC on HUB    = BRIDGE MAC   <- not the hub's own MAC
ESPNOW_PMK        on BOTH   = same secret
```

### 3. Open the dashboard

1. Read the IP on the TTGO screen (or Serial @ 115200 baud).
2. Browse to:

```text
http://<esp-ip>:6789/
```

Binary telemetry API (decoded in the browser; **requires login session cookie**):

```text
http://<esp-ip>:6789/api/telemetry.bin
```

### Web authentication

- First visit with empty NVS: browser asks for username/password → ESP screen asks for **physical confirm** (**TOP / GPIO35 = OK**, **BOTTOM / GPIO0 = Cancel**).
- Username ≤32 `[A-Za-z0-9._-]`; password 8–64 printable ASCII. Auth JSON bodies capped at 512 bytes (overflow / injection hardening).
- After credentials are stored (salted SHA-256 in NVS `wg_auth`), the SPA stays on the login screen until `/api/auth/login` succeeds. Telemetry is rejected with **401** without a valid `wg_session` cookie (`HttpOnly; SameSite=Strict`).
- Forgot password / Account tab: hold **BOTTOM (GPIO0) for 3 seconds** → confirm clear on TOP. Then recreate credentials. Use **Account** to sign out.
- Threat model: HTTP on LAN + local password. Sessions are RAM-only (re-login after reboot). Do **not** expose port 6789 to the public internet.
- **HTTPS note:** on this classic ESP32 + NimBLE stack, in-process TLS (`esp32_https_server` / OpenSSL) freezes the device under load — HTTP is intentional until a lighter TLS path exists.

Web sources live in `web/` (HTML views + CSS + JS modules). `scripts/bundle_web.ps1` packs them into `data/` before LittleFS upload (ETag caching on static assets). The `data/` folder is gitignored — regenerate with the bundle script / PlatformIO pre-script.

TTGO buttons: **GPIO0 (bottom)** = previous page / Cancel; **GPIO35 (top)** = next page / Validate. No touchscreen. Backlight sleeps after **60 s** without input; the display UI task is suspended while asleep and resumes on the next button press (wake only, no page change). Gateway/overview show **`ip:port`** for the web UI.

### Web UI screenshots

Each image matches one dashboard tab (same order as the SPA nav).

<table>
  <tr>
    <td align="center" width="50%">
      <p><strong>Overview</strong></p>
      <img src="Resources/overview.png" alt="Overview tab — SoC, live metrics and charts" width="100%" />
    </td>
    <td align="center" width="50%">
      <p><strong>Cells</strong></p>
      <img src="Resources/Cells.jpg" alt="Cells tab — per-cell voltages" width="100%" />
    </td>
  </tr>
  <tr>
    <td align="center" width="50%">
      <p><strong>Temps</strong></p>
      <img src="Resources/Temps.jpg" alt="Temps tab — MOS / PCB / cell temperatures" width="100%" />
    </td>
    <td align="center" width="50%">
      <p><strong>Alerts</strong></p>
      <img src="Resources/Alerts.jpg" alt="Alerts tab — BMS warning flags" width="100%" />
    </td>
  </tr>
  <tr>
    <td align="center" width="50%">
      <p><strong>Gateway</strong></p>
      <img src="Resources/Gateway.jpg" alt="Gateway tab — Wi-Fi / BLE / poll status" width="100%" />
    </td>
    <td align="center" width="50%">
      <p><strong>ESP</strong></p>
      <img src="Resources/ESP.png" alt="ESP tab — CPU, heap, chip temperature" width="100%" />
    </td>
  </tr>
  <tr>
    <td align="center" colspan="2">
      <p><strong>Account</strong></p>
      <img src="Resources/Account.jpg" alt="Account tab — logout and credential reset help" width="40%" />
    </td>
  </tr>
</table>

### 4. Later updates over the air

```powershell
.\scripts\flash_ota.ps1 -Hostname wattcycle-gateway
# or, if mDNS is flaky on Windows:
.\scripts\flash_ota.ps1 -Ip 192.168.x.y
```

### Manual PlatformIO commands

```powershell
pio test -e native
pio run -e hub_ttgo_wattcycle
pio run -e bridge_xt369p
pio run -e hub_ttgo_wattcycle -t upload --upload-port COM19
pio run -e hub_ttgo_wattcycle -t uploadfs --upload-port COM19
pio run -e bridge_xt369p -t upload --upload-port COM22
pio device monitor -p COM19 -b 115200
```

---

## Configuration reference

| Variable / flag | Purpose | Target |
|-----------------|---------|--------|
| `WIFI_SSID` / `WIFI_PASS` | Hub STA credentials | hub |
| `WIFI_SSID` | Bridge channel discovery (no join) | bridge |
| `BMS_BLE_ADDRESS` | Wattcycle BLE MAC | hub |
| `XT369P_BT_ADDRESS` | Optional Classic BT MAC (else name `XT369P_SPP`) | bridge |
| `ESPNOW_BRIDGE_MAC` | Bridge STA MAC (RX peer) | hub |
| `ESPNOW_PEER_MAC` | Hub STA MAC (TX peer) | bridge |
| `ESPNOW_PMK` | Shared 32-char hex secret | both |
| `WEB_SERVER_PORT` | Default **6789** | hub |
| `BMS_POLL_INTERVAL_MS` | Default **2000** | hub |
| `OTA_HOSTNAME` | Default `wattcycle-gateway` | hub |

---

## Architecture notes

```mermaid
classDiagram
  class IBmsBleClient
  class ITelemetryStore
  class IWifiConnector
  class IOtaUpdater
  class IWebGateway
  class IStatusDisplay
  class IAuthSessionService
  class IAuthPhysicalConfirm
  class GatewayApplication
  class TelemetryPoller

  GatewayApplication --> IWifiConnector
  GatewayApplication --> IOtaUpdater
  GatewayApplication --> IWebGateway
  GatewayApplication --> IStatusDisplay
  GatewayApplication --> IAuthPhysicalConfirm
  GatewayApplication --> TelemetryPoller
  TelemetryPoller --> IBmsBleClient
  TelemetryPoller --> ITelemetryStore
  IWebGateway --> ITelemetryStore
  IWebGateway --> IAuthSessionService
  IStatusDisplay ..> AuthPrompt : renders
```

`src/main.cpp` constructs concrete adapters and injects them into `GatewayApplication` (interfaces only); auth `begin()` runs in `main` before the composition root.
Auth is ISP-split: `EspWebGateway` depends on `IAuthSessionService` (login/setup start only — physical confirm/reset stay private on `AuthService`); display/buttons use `IAuthPhysicalConfirm` + shared `auth::AuthPrompt`.
Browser chart history is **localStorage** (up to 24 h): live samples append continuously, and the History tab can **merge today’s SD CSV** (UTC day files under `/history/`) into that store so charts are complete after login. Picking another day in the list loads that CSV into the charts.
M5 overview clock uses local time via `POSIX_TIMEZONE` (default Europe/Paris `CET-1CEST,M3.5.0,M10.5.0/3`); SD filenames stay UTC.
`scripts/check_guardrails.ps1` fails if files grow past limits or concrete adapter headers leak outside `main.cpp` / their domain.

---

## Credits & references

- Protocol: [qume/wattcycle_ble](https://github.com/qume/wattcycle_ble)
- Related ecosystem: ioBroker.wattcycle, dbus-serialbattery XDZN BLE driver discussions
- Display: LILYGO TTGO T-Display / ST7789V community pinouts

---

## License

See [LICENSE](LICENSE).
