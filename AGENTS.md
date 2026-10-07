# Agent Rules — Wattcycle ESP32 Gateway

This file is the contract for humans and coding agents working in this repository.

## Mission

Build and maintain a **BLE → Wi-Fi web gateway** for Wattcycle / XDZN BMS packs
(51.2 V class) on a **LILYGO TTGO T-Display (ESP32-D0WDQ6, ST7789V 1.14")**.

The gateway:

1. Connects to the BMS over Bluetooth Low Energy (no pairing).
2. Polls telemetry using the reverse-engineered Wattcycle protocol.
3. Serves a static dashboard from **LittleFS** on TCP port **6789**.
4. Supports **ArduinoOTA** updates after the first USB flash.

## Non-negotiable rules

### Security

- **Never** hardcode Wi-Fi credentials, OTA passwords, or BMS secrets in source.
- Inject secrets only via environment variables consumed by PlatformIO:
  - `WIFI_SSID`
  - `WIFI_PASS`
  - `BMS_BLE_ADDRESS` (optional but required for BLE connect)
- Do not commit `.env` files, private keys, or captured telemetry dumps with PII.

### Architecture (SOLID + DI)

- Prefer **interfaces** (`I*`) at domain boundaries.
- Construct concrete adapters only in `src/main.cpp`; `GatewayApplication` depends on interfaces.
- Keep domain folders focused:
  - `Bms/` protocol + BLE transport
  - `Telemetry/` store + polling + JSON + gateway status
  - `Wifi/`, `Ota/`, `Web/`, `Display/`, `Config/`, `CompositionRoot/`
- No nested classes.
- No god-objects: **≤ 400 lines per file**, **≤ 30 methods per class**.
- Run `scripts/check_guardrails.ps1` (also invoked by `scripts/run_tests.ps1`).

### Quality gates after meaningful changes

1. `.\scripts\run_tests.ps1`
2. Thermo-nuclear code-quality review (blocking findings must be fixed before merge).
3. Update `README.md` when behavior, scripts, hardware assumptions, or architecture change.

### Platform / hardware assumptions

- Target MCU is classic **ESP32** (`ESP32-D0WDQ6`), **not** ESP32-S3.
- Default USB port in scripts/config: **COM19**.
- Display driver: ST7789V pins MOSI=19, SCLK=18, CS=5, DC=16, RST=23, BL=4.
- Protocol reference: [qume/wattcycle_ble](https://github.com/qume/wattcycle_ble) (`PROTOCOL.md`).

### Documentation style

- README is **English**.
- Prefer Mermaid diagrams and short Quick Start command blocks.
- Explain *why* (no cloud on the BMS → local gateway) before *how*.

### Coding style

- Clear, pronounceable names.
- Headers declare intent; `.cpp` files stay boring and direct.
- Prefer small pure parsers (CRC, frames, analog payload) that are unit-tested on `native`.
- Hardware adapters (`Wifi`, `Ble`, `Web`, `Display`, `Ota`) stay behind interfaces.

## Useful commands

```powershell
$env:WIFI_SSID = "your-ssid"
$env:WIFI_PASS = "your-password"
$env:BMS_BLE_ADDRESS = "AA:BB:CC:DD:EE:FF"

.\scripts\run_tests.ps1
.\scripts\flash_usb.ps1 -Port COM19
.\scripts\flash_ota.ps1 -Hostname wattcycle-gateway
```

## Out of scope (unless explicitly requested)

- Writing charge/discharge control commands to the BMS.
- Storing long-term history (no SD card — keep the design ephemeral + live JSON).
- Migrating to ESP32-S3 / different display boards without an architecture note.
