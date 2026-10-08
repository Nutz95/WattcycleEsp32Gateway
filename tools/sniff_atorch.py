#!/usr/bin/env python3
"""Sniff / decode ATorch XT369P SPP telemetry (Windows Bluetooth COM)."""

from __future__ import annotations

import argparse
import json
import sys
import time
from dataclasses import asdict, dataclass
from pathlib import Path

import serial
import serial.tools.list_ports

MAGIC = b"\xff\x55"
REPORT_LEN = 36


@dataclass
class DcReading:
    voltage_v: float
    current_a: float
    power_w: float
    capacity_ah: float
    energy_wh: float
    temperature_c: float
    runtime_s: int
    backlight_s: int
    price_per_kwh: float
    checksum_ok: bool
    raw_hex: str


def checksum_std(frame: bytes) -> int:
    """Classic ATorch CRC: sum(bytes[2:-1]) ^ 0x44."""
    return (sum(frame[2:-1]) & 0xFF) ^ 0x44


def checksum_xt369p(frame: bytes) -> int:
    """XT369P variant observed on COM20: sum(bytes[3:-1]) ^ 0x44."""
    return (sum(frame[3:-1]) & 0xFF) ^ 0x44


def be24(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 3], "big")


def be32(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 4], "big")


def be16(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 2], "big")


def decode_dc_report(frame: bytes) -> DcReading | None:
    """Decode a 36-byte DC meter report (device type 0x02).

    Layout matches esphome-atorch-dl24 decode_ac_and_dc_().
    """
    if len(frame) != REPORT_LEN or frame[:2] != MAGIC:
        return None
    if frame[2] != 0x01 or frame[3] != 0x02:
        return None

    voltage = be24(frame, 4) * 0.1
    current = be24(frame, 7) * 0.001
    capacity = be24(frame, 10) * 0.01
    energy_raw = be32(frame, 13)
    energy = energy_raw / 100.0
    if energy_raw == 0 and capacity > 0 and voltage > 0.5:
        energy = capacity * voltage
    price = be24(frame, 17) * 0.01
    temperature = float(be16(frame, 24))
    runtime = be16(frame, 26) * 3600 + frame[28] * 60 + frame[29]
    backlight = frame[30]
    ck_ok = frame[-1] in (checksum_std(frame), checksum_xt369p(frame))

    return DcReading(
        voltage_v=round(voltage, 3),
        current_a=round(current, 3),
        power_w=round(voltage * current, 3),
        capacity_ah=round(capacity, 3),
        energy_wh=round(energy, 1),
        temperature_c=temperature,
        runtime_s=runtime,
        backlight_s=backlight,
        price_per_kwh=round(price, 2),
        checksum_ok=ck_ok,
        raw_hex=frame.hex(" "),
    )


def extract_reports(buf: bytearray) -> list[bytes]:
    frames: list[bytes] = []
    while True:
        idx = buf.find(MAGIC)
        if idx < 0:
            buf.clear()
            return frames
        if idx:
            del buf[:idx]
        if len(buf) < 3:
            return frames
        msg = buf[2]
        need = {0x01: 36, 0x02: 8, 0x11: 10}.get(msg)
        if need is None:
            del buf[0]
            continue
        if len(buf) < need:
            return frames
        if msg == 0x01:
            frames.append(bytes(buf[:need]))
        del buf[:need]


def list_ports() -> None:
    for p in serial.tools.list_ports.comports():
        print(f"{p.device}: {p.description} | {p.hwid}")


def find_xt369p_port() -> str | None:
    """Prefer COM whose Bluetooth address matches XT369P (…0312AE…)."""
    for p in serial.tools.list_ports.comports():
        if "0000000312AE" in p.hwid.upper() or "0312AE" in p.hwid.upper():
            return p.device
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description="XT369P / ATorch SPP sniffer")
    parser.add_argument("--port", default=None, help="COM port (auto-detect if omitted)")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=0.0, help="0 = run until Ctrl+C")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--dump", type=Path, default=None)
    parser.add_argument("--json", action="store_true", help="One JSON object per reading")
    args = parser.parse_args()

    if args.list:
        list_ports()
        return 0

    port = args.port or find_xt369p_port() or "COM20"
    print(f"Opening {port} @ {args.baud} ...", flush=True)
    try:
        # timeout=1: Windows BT SPP often needs several seconds before first bytes
        ser = serial.Serial(port, args.baud, timeout=1.0, write_timeout=5)
    except serial.SerialException as exc:
        print(f"Failed: {exc}", file=sys.stderr)
        list_ports()
        return 1

    print(
        "Listening (device pushes ~1 Hz DC reports). "
        "First frame can take 5–15s after COM open. Ctrl+C to stop.",
        flush=True,
    )
    buf = bytearray()
    raw_all = bytearray()
    deadline = time.time() + args.seconds if args.seconds > 0 else None
    count = 0
    linked = False

    try:
        while deadline is None or time.time() < deadline:
            chunk = ser.read(max(ser.in_waiting, 1))
            if not chunk:
                continue
            if not linked:
                linked = True
                print("SPP link up — decoding…", flush=True)
            raw_all.extend(chunk)
            buf.extend(chunk)
            for frame in extract_reports(buf):
                reading = decode_dc_report(frame)
                if reading is None:
                    print(f"non-DC frame: {frame.hex(' ')}", flush=True)
                    continue
                count += 1
                if args.json:
                    print(json.dumps(asdict(reading)), flush=True)
                else:
                    ck = "ok" if reading.checksum_ok else "bad"
                    print(
                        f"{reading.voltage_v:7.2f} V | "
                        f"{reading.current_a:7.3f} A | "
                        f"{reading.power_w:8.2f} W | "
                        f"{reading.capacity_ah:8.2f} Ah | "
                        f"{reading.energy_wh:8.1f} Wh | "
                        f"{reading.temperature_c:5.0f} °C | "
                        f"run {reading.runtime_s // 3600:02d}:"
                        f"{(reading.runtime_s % 3600) // 60:02d}:"
                        f"{reading.runtime_s % 60:02d} | ck={ck}",
                        flush=True,
                    )
    except KeyboardInterrupt:
        print("\nStopped.", flush=True)
    finally:
        ser.close()
        if args.dump is not None:
            args.dump.write_bytes(raw_all)
            print(f"Wrote {len(raw_all)} bytes -> {args.dump}", flush=True)
        print(f"Readings: {count}", flush=True)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
