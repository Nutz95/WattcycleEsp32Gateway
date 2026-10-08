# ATorch XT369P — Bluetooth SPP protocol

Solar wattmeter transport used by the `bridge_xt369p` firmware in this monorepo.
The hub dashboard / auth / history live on `hub_ttgo_wattcycle` (ESP-NOW RX).

<p align="center">
  <img src="../Resources/xt369p.png" alt="ATorch XT369P wattmeter" width="480" />
</p>

<p align="center">
  <img src="../Resources/xt369p_front_back.png" alt="XT369P front and back" width="640" />
</p>

## Transport

| Item | Value |
|------|--------|
| BT name | `XT369P_SPP` |
| Profile | Bluetooth Classic **SPP** (RFCOMM → Windows COM port) |
| Address | Read from Windows Bluetooth properties (e.g. `AA:BB:…`) |
| COM port (example) | Outgoing SPP whose HWID contains the meter address |
| Baud (virtual) | 115200 8N1 (transparent SPP) |
| Stream | Meter **pushes** reports ~1 Hz after connect (no poll) |

Same protocol family as UD18 / DT24 / DL24 (NiceLabs / ESPHome docs).

## Report frame (36 bytes)

```
FF 55 01 02 .... .... CK
│  │  │  │           └─ checksum
│  │  │  └─ device type 0x02 = DC meter
│  │  └─ message type 0x01 = report
│  └─ magic
└─ magic
```

### DC fields (aligned with `esphome-atorch-dl24`)

| Offset | Size | Field | Scale |
|-------:|-----:|-------|-------|
| 4 | 3 | Voltage | `u24 BE * 0.1` → V |
| 7 | 3 | Current | `u24 BE * 0.001` → A |
| 10 | 3 | Capacity | `u24 BE * 0.01` → Ah |
| 13 | 4 | Energy | `u32 BE` — scale varies by firmware (`*10` ESPHome DC, `/100` NiceLabs, `/1000` fine); decoder picks closest to Ah×V (raw `0` → Ah×V) |
| 17 | 3 | Price / kWh | `u24 BE * 0.01` |
| 20 | 4 | (unknown / reserved) | |
| 24 | 2 | Temperature | `u16 BE` → °C |
| 26 | 2 | Runtime hours | |
| 28 | 1 | Minutes | |
| 29 | 1 | Seconds | |
| 30 | 1 | Backlight (s) | |
| 31 | 4 | padding | |
| 35 | 1 | Checksum | |

**Power** = V × I (no dedicated watt field in DC mode, unlike older NiceLabs AC docs).

### Checksum

Classic ATorch algorithm:

```text
CK = (sum(frame[2 .. -2]) & 0xFF) ^ 0x44
```

On captured XT369P units, the classic CRC is often **off-by-one**; the matching variant is:

```text
CK = (sum(frame[3 .. -2]) & 0xFF) ^ 0x44
```

→ accept both, or disable verification (like ESPHome `check_crc: false` on some models).

## Commands

Type `0x11`, 10 bytes: `FF 55 11 <device> <cmd> <u32 BE value> CK`

| Cmd | Action |
|----:|--------|
| 0x01 | Reset Wh |
| 0x02 | Reset Ah |
| 0x03 | Reset duration |
| 0x05 | Reset all |

Hub can forward these over ESP-NOW (`EspNowCommandV1`) from the Solar tab.

## References

- https://github.com/NiceLabs/atorch-console/blob/master/docs/protocol-design.md
- https://github.com/syssi/esphome-atorch-dl24
- https://sigrok.org/wiki/ATORCH_J7-c
