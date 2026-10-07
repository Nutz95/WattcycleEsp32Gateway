/* Wattcycle gateway binary telemetry decoder (little-endian, version 1). */
(function (global) {
  const MAGIC = 0x4d475457; // WTGM

  function u8(view, offset) { return view.getUint8(offset); }
  function u16(view, offset) { return view.getUint16(offset, true); }
  function i16(view, offset) { return view.getInt16(offset, true); }
  function u32(view, offset) { return view.getUint32(offset, true); }

  function readFixedString(bytes, offset, length) {
    let end = offset;
    const limit = offset + length;
    while (end < limit && bytes[end] !== 0) end += 1;
    return new TextDecoder().decode(bytes.subarray(offset, end));
  }

  function decodeTelemetry(buffer) {
    const bytes = new Uint8Array(buffer);
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    if (bytes.length < 32 || u32(view, 0) !== MAGIC) {
      throw new Error("Invalid telemetry magic");
    }
    const version = u8(view, 4);
    if (version !== 1) {
      throw new Error("Unsupported telemetry version " + version);
    }

    let o = 5;
    const flags = u8(view, o); o += 1;
    const data = {
      valid: (flags & 0x01) !== 0,
      gateway: {
        wifi: (flags & 0x02) !== 0,
        ble: (flags & 0x04) !== 0,
        telemetryFresh: (flags & 0x20) !== 0
      },
      warnings: {
        protection: (flags & 0x08) !== 0,
        fault: (flags & 0x10) !== 0
      }
    };

    data.soc = u8(view, o); o += 1;
    data.soh = u8(view, o); o += 1;
    data.voltage = u16(view, o) / 100; o += 2;
    data.current = i16(view, o) / 10; o += 2;
    data.power = i16(view, o); o += 2;
    data.remainingAh = u16(view, o) / 10; o += 2;
    data.totalAh = u16(view, o) / 10; o += 2;
    data.designAh = u16(view, o) / 10; o += 2;
    data.cycles = u16(view, o); o += 2;
    data.mosTemp = i16(view, o) / 10; o += 2;
    data.pcbTemp = i16(view, o) / 10; o += 2;
    data.updatedAtMs = u32(view, o); o += 4;

    const cellCount = u8(view, o); o += 1;
    data.cells = [];
    for (let i = 0; i < cellCount; i += 1) {
      data.cells.push(u16(view, o) / 1000);
      o += 2;
    }

    const cellTempCount = u8(view, o); o += 1;
    data.cellTemps = [];
    for (let i = 0; i < cellTempCount; i += 1) {
      data.cellTemps.push(i16(view, o) / 10);
      o += 2;
    }

    data.warnings.status1 = u8(view, o); o += 1;
    data.warnings.status2 = u8(view, o); o += 1;
    data.warnings.status5 = u8(view, o); o += 1;
    data.warnings.warn1 = u8(view, o); o += 1;
    data.warnings.warn2 = u8(view, o); o += 1;

    data.product = {
      fw: readFixedString(bytes, o, 20),
      mfr: readFixedString(bytes, o + 20, 20),
      sn: readFixedString(bytes, o + 40, 20)
    };
    o += 60;
    data.gateway.ip = readFixedString(bytes, o, 16); o += 16;
    data.gateway.bleAddress = readFixedString(bytes, o, 18); o += 18;
    data.gateway.error = readFixedString(bytes, o, 64);

    return data;
  }

  global.WattcycleBinary = { decodeTelemetry: decodeTelemetry };
})(window);
