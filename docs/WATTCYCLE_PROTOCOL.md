# Wattcycle / XDZN BMS — BLE protocol (summary)

Local summary of the reverse-engineered Wattcycle BLE protocol used by
`hub_ttgo_wattcycle`. Full upstream notes:
[`qume/wattcycle_ble` PROTOCOL.md](https://github.com/qume/wattcycle_ble/blob/main/PROTOCOL.md).

**OEM:** XDZN · **Name prefix:** `XDZN` / `WT` · **No pairing required**

## GATT

| Role | UUID |
|------|------|
| Service | `0xFFF0` |
| Notify | `0xFFF1` |
| Write | `0xFFF2` |
| Auth | `0xFFFA` |

## Authentication

After connect, write ASCII `HiLink` (`48 69 4C 69 6E 6B`) to **FFFA**.

## Frame format

Big-endian Modbus-like frames:

```text
[HEAD 0x7E|0x1E] [VER] [ADDR 0x01] [FUNC] [START:2] [LEN/COUNT:2] [DATA…] [CRC16:2] [TAIL 0x0D]
```

- FUNC `0x03` = read, `0x06` = write  
- CRC16 = standard Modbus over bytes from HEAD through last DATA byte  
- BLE MTU splits responses; reassemble until `DATA_LEN + 11` bytes

## Main data points used by this gateway

| DP | Hex | Name | Use here |
|----|-----|------|----------|
| 140 | `0x8C` | Analog Quantity | SoC, V, I, cells, temps, Ah |
| 141 | `0x8D` | Warning Info | Protection / fault / balance bits |
| 146 | `0x92` | Product Info | FW / manufacturer / serial (60 B ASCII) |

### Analog Quantity (high level)

1. `cellCount` + `cellCount × u16` cell voltages (`/1000` → V)  
2. `temperatureCount` + MOS/PCB temps (`(raw - 2730) / 10` → °C) + cell temps  
3. Signed current (custom 2-byte Watt encoding) + module voltage + Ah + cycles + SoC  
4. Optional extension: SoH, cumulative Ah, balance current  

This firmware is **monitoring only** — charge/discharge control DPs are out of scope.

## Connection sequence (gateway)

1. Connect to `BMS_BLE_ADDRESS` (no scan required when MAC is set)  
2. Enable notify on FFF1  
3. Auth `HiLink` on FFFA  
4. Poll Analog / Warning / Product on the configured interval  
5. Encode a fused binary snapshot for the web UI (`BinaryTelemetryCodec`)

## See also

- Upstream deep dive: https://github.com/qume/wattcycle_ble/blob/main/PROTOCOL.md  
- Solar meter protocol: [XT369P_PROTOCOL.md](XT369P_PROTOCOL.md)  
- ESP-NOW wire header: `src/common/EspNow/Xt369pEspNowProtocol.h`
