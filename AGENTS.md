# Agent Rules — Wattcycle ESP32 Gateway (monorepo)

This file is the contract for humans and coding agents working in this repository.

## Mission

Maintain a **monorepo** (Phase 2 cutover in progress):

1. **`hub_m5_wattcycle`** (COM23) — M5Stack Basic web hub (:6789) + multi-peer ESP-NOW RX (BMS + solar) + auth/OTA.
2. **`bridge_ttgo_wattcycle`** (COM19) — Wattcycle / XDZN BMS over BLE → ESP-NOW TX (no web / auth / OTA).
3. **`bridge_xt369p`** (COM22) — ATorch XT369P over Classic SPP → ESP-NOW TX (no web / auth / OTA).
4. **`hub_ttgo_wattcycle`** (legacy) — Phase 1 combined hub; keep until M5 cutover is verified (tag V1.0.0 rollback).

Shared wire protocols and utils live in `src/common/` (`Xt369pEspNowProtocol`, `WattcycleEspNowProtocol`).

## Non-negotiable rules

### Security

- **Never** hardcode Wi-Fi credentials, OTA passwords, web passwords, BMS secrets, or ESP-NOW PMK in source.
- Inject secrets only via environment variables consumed by PlatformIO:
  - M5 hub: `WIFI_SSID`, `WIFI_PASS`, `ESPNOW_BMS_BRIDGE_MAC`, `ESPNOW_BRIDGE_MAC` (XT), `ESPNOW_PMK`
  - BMS bridge: `WIFI_SSID` (channel), `BMS_BLE_ADDRESS`, `ESPNOW_PEER_MAC`, `ESPNOW_PMK`
  - XT bridge: `WIFI_SSID` (channel), `XT369P_BT_ADDRESS`, `ESPNOW_PEER_MAC`, `ESPNOW_PMK`
  - Legacy TTGO hub: also `BMS_BLE_ADDRESS` + single `ESPNOW_BRIDGE_MAC`
- Web UI auth (hub only): salted SHA-256 in NVS (`wg_auth`); RAM sessions (max 4; oldest-expiring eviction); telemetry APIs gated server-side.
- Auth ISP: `IAuthSessionService` (web) + `IAuthPhysicalConfirm` (buttons/display); physical confirm/reset private on `AuthService`.
- HTTP on LAN is intentional on the hub (classic ESP32 + NimBLE). Do not expose port 6789 publicly.
- Do not commit `.env` files, private keys, or telemetry dumps with PII.

### Architecture (SOLID + DI)

- Prefer **interfaces** (`I*`) at domain boundaries.
- Construct concrete adapters only in each target’s `main.cpp`; composition roots depend on interfaces.
- Layout:
  - `src/common/` — `Util/`, `EspNow/{Xt369p,Wattcycle}EspNowProtocol.h`
  - `src/hub_m5_wattcycle/` — Auth, Web, Wifi, Ota, multi-peer EspNow RX, M5 display, Telemetry
  - `src/bridge_ttgo_wattcycle/` — Bms BLE, EspNow TX, TTGO display — **no** Auth/Web/Ota
  - `src/bridge_xt369p/` — Meter/SPP, EspNow TX, Display — **no** Auth/Web/Ota
  - `src/hub_ttgo_wattcycle/` — legacy Phase 1 hub
  - `web/` SPA → `scripts/bundle_web.ps1` → LittleFS `data/` (hub only)
- Browser chart history is localStorage-only (Phase 1).
- No nested classes; **≤ 400 lines per file**, **≤ 30 methods per class**.
- Run `scripts/check_guardrails.ps1` (also via `scripts/run_tests.ps1`).

### Quality gates after meaningful changes

1. `.\scripts\run_tests.ps1`
2. Thermo-nuclear code-quality review (blocking findings must be fixed before merge).
3. Update `README.md` when behavior, scripts, hardware assumptions, or architecture change.

### Platform / hardware assumptions

- Classic **ESP32** (`ESP32-D0WDQ6`), **not** ESP32-S3 (Phase 1 targets).
- Default ports: M5 hub **COM23**, BMS bridge **COM19**, XT bridge **COM22**.
- TTGO display: ST7789V pins MOSI=19, SCLK=18, CS=5, DC=16, RST=23, BL=4.
- Protocol refs: [qume/wattcycle_ble](https://github.com/qume/wattcycle_ble); `docs/XT369P_PROTOCOL.md`.

### Documentation style

- README is **English**.
- Prefer Mermaid diagrams and short Quick Start command blocks.

### Coding style

- Clear, pronounceable names.
- Headers declare intent; `.cpp` files stay boring and direct.
- Prefer small pure parsers unit-tested on `native`.
- Hardware adapters stay behind interfaces.

## Useful commands

```powershell
$env:WIFI_SSID = "your-ssid"
$env:WIFI_PASS = "your-password"
$env:BMS_BLE_ADDRESS = "AA:BB:CC:DD:EE:FF"

.\scripts\run_tests.ps1
.\scripts\flash.ps1 -Target HubTtgoWattcycle -Port COM19
.\scripts\flash.ps1 -Target BridgeXt369p -Port COM22
.\scripts\flash.ps1 -Target Both -HubPort COM19 -BridgePort COM22
.\scripts\pair_espnow_link.ps1 -HubPort COM19 -BridgePort COM22 -Flash
.\scripts\flash_ota.ps1 -Hostname wattcycle-gateway
```

## Out of scope (unless explicitly requested)

- Writing charge/discharge control commands to the BMS.
- Storing long-term history on the TTGO hub (Phase 2 M5 + SD).
- Migrating hub to M5 / ESP32-P4 without following the Phase 2 HAL plan.
- Shipping in-process HTTPS on classic ESP32 + NimBLE until a lighter TLS path exists.
