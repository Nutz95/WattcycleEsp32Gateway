/* Wattcycle gateway binary telemetry decoder (little-endian). */
(function (global) {
  const MAGIC = 0x4d475457;
  const VERSION = 5;

  function u8(view, offset) { return view.getUint8(offset); }
  function u16(view, offset) { return view.getUint16(offset, true); }
  function i16(view, offset) { return view.getInt16(offset, true); }
  function u32(view, offset) { return view.getUint32(offset, true); }
  function i32(view, offset) { return view.getInt32(offset, true); }

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
    if (version !== VERSION) {
      throw new Error("Unsupported telemetry version " + version);
    }

    let offset = 5;
    const flags = u8(view, offset); offset += 1;
    const data = {
      version: version,
      valid: (flags & 0x01) !== 0,
      gateway: {
        wifi: (flags & 0x02) !== 0,
        ble: (flags & 0x04) !== 0,
        telemetryFresh: (flags & 0x20) !== 0,
        solarLink: (flags & 0x40) !== 0,
        spp: (flags & 0x80) !== 0
      },
      warnings: {
        protection: (flags & 0x08) !== 0,
        fault: (flags & 0x10) !== 0
      },
      esp: {},
      solar: {},
      balancing: []
    };

    data.soc = u8(view, offset); offset += 1;
    data.soh = u8(view, offset); offset += 1;
    data.voltage = u16(view, offset) / 100; offset += 2;
    data.current = i16(view, offset) / 10; offset += 2;
    data.power = i16(view, offset); offset += 2;
    data.balanceCurrent = i16(view, offset) / 10; offset += 2;
    data.remainingAh = u16(view, offset) / 10; offset += 2;
    data.totalAh = u16(view, offset) / 10; offset += 2;
    data.designAh = u16(view, offset) / 10; offset += 2;
    data.cycles = u16(view, offset); offset += 2;
    data.mosTemp = i16(view, offset) / 10; offset += 2;
    data.pcbTemp = i16(view, offset) / 10; offset += 2;
    data.updatedAtMs = u32(view, offset); offset += 4;

    const cellCount = u8(view, offset); offset += 1;
    data.cells = [];
    for (let i = 0; i < cellCount; i += 1) {
      data.cells.push(u16(view, offset) / 1000);
      offset += 2;
    }

    const balanceByteCount = Math.ceil(cellCount / 8);
    for (let i = 0; i < cellCount; i += 1) {
      data.balancing[i] = false;
    }
    for (let byteIndex = 0; byteIndex < balanceByteCount; byteIndex += 1) {
      const bits = u8(view, offset); offset += 1;
      for (let bit = 0; bit < 8; bit += 1) {
        const cellIndex = byteIndex * 8 + bit;
        if (cellIndex < cellCount) {
          data.balancing[cellIndex] = ((bits >> bit) & 1) === 1;
        }
      }
    }

    const cellTempCount = u8(view, offset); offset += 1;
    data.cellTemps = [];
    for (let i = 0; i < cellTempCount; i += 1) {
      data.cellTemps.push(i16(view, offset) / 10);
      offset += 2;
    }

    data.warnings.status1 = u8(view, offset); offset += 1;
    data.warnings.status2 = u8(view, offset); offset += 1;
    data.warnings.status5 = u8(view, offset); offset += 1;
    data.warnings.warn1 = u8(view, offset); offset += 1;
    data.warnings.warn2 = u8(view, offset); offset += 1;

    data.product = {
      fw: readFixedString(bytes, offset, 20),
      mfr: readFixedString(bytes, offset + 20, 20),
      sn: readFixedString(bytes, offset + 40, 20)
    };
    offset += 60;
    data.gateway.ip = readFixedString(bytes, offset, 16); offset += 16;
    data.gateway.bleAddress = readFixedString(bytes, offset, 18); offset += 18;
    data.gateway.error = readFixedString(bytes, offset, 64); offset += 64;

    data.esp.cpu0 = u8(view, offset); offset += 1;
    data.esp.cpu1 = u8(view, offset); offset += 1;
    data.esp.chipTemp = i16(view, offset) / 10; offset += 2;
    data.esp.heapKb = u16(view, offset); offset += 2;
    data.esp.uptimeSec = u32(view, offset); offset += 4;

    // v4 fixed solar block + bridge ESP health trailer (fail closed).
    const SOLAR_BLOCK_BYTES = 73;
    const BRIDGE_ESP_BYTES = 10;
    const BMS_BRIDGE_TRAILER = 11;
    if (offset + SOLAR_BLOCK_BYTES + BRIDGE_ESP_BYTES + BMS_BRIDGE_TRAILER > bytes.length) {
      throw new Error("Truncated telemetry solar/bridge trailer");
    }
    const solarFlags = u8(view, offset); offset += 1;
    data.solar.valid = (solarFlags & 0x01) !== 0;
    data.solar.checksumOk = (solarFlags & 0x02) !== 0;
    data.solar.espNowEncrypted = (solarFlags & 0x04) !== 0;
    data.solar.bridgeEspValid = (solarFlags & 0x08) !== 0;
    data.solar.voltage = u16(view, offset) / 100; offset += 2;
    data.solar.current = i32(view, offset) / 1000; offset += 4;
    data.solar.power = i32(view, offset) / 100; offset += 4;
    data.solar.capacityAh = u32(view, offset) / 1000; offset += 4;
    data.solar.energyWh = u32(view, offset) / 1000; offset += 4;
    data.solar.tempC = i16(view, offset) / 10; offset += 2;
    data.solar.runtimeS = u32(view, offset); offset += 4;
    data.solar.frameCount = u32(view, offset); offset += 4;
    data.solar.seq = u32(view, offset); offset += 4;
    data.solar.target = readFixedString(bytes, offset, 16); offset += 16;
    data.solar.error = readFixedString(bytes, offset, 24); offset += 24;
    data.solar.linkFresh = data.gateway.solarLink;
    data.solar.sppConnected = data.gateway.spp;
    data.gateway.espNowEncrypted = data.solar.espNowEncrypted;

    data.bridgeEsp = {
      cpu0: u8(view, offset),
      cpu1: u8(view, offset + 1),
      chipTemp: i16(view, offset + 2) / 10,
      heapKb: u16(view, offset + 4),
      uptimeSec: u32(view, offset + 6),
      valid: data.solar.bridgeEspValid
    };
    offset += BRIDGE_ESP_BYTES;

    // v5: BMS BLE bridge ESP health trailer
    const BMS_BRIDGE_ESP_BYTES = 11;
    if (offset + BMS_BRIDGE_ESP_BYTES > bytes.length) {
      throw new Error("Truncated telemetry BMS-bridge trailer");
    }
    const bmsBridgeFlags = u8(view, offset); offset += 1;
    data.gateway.ntp = (bmsBridgeFlags & 0x02) !== 0;
    data.bmsBridgeEsp = {
      cpu0: u8(view, offset),
      cpu1: u8(view, offset + 1),
      chipTemp: i16(view, offset + 2) / 10,
      heapKb: u16(view, offset + 4),
      uptimeSec: u32(view, offset + 6),
      valid: (bmsBridgeFlags & 0x01) !== 0
    };
    offset += 10;

    return data;
  }

  global.WattcycleBinary = { decodeTelemetry: decodeTelemetry };
})(window);
