/* Wattcycle gateway binary telemetry decoder (little-endian). */
(function (global) {
  const MAGIC = 0x4d475457;
  const VERSION = 9;
  const VOLT_DECI = 10;
  const AMP_CENTI = 100;
  // Mirrors ecoflow_bridge::ExtraFlags / RuntimeFlags.
  const EXTRA_AC_INPUT_ON = 1 << 0;
  const EXTRA_PV_INPUT_ON = 1 << 1;
  const EXTRA_CELL_TEMP = 1 << 2;
  const EXTRA_SOH = 1 << 3;
  const EXTRA_FREQ = 1 << 4;
  const EXTRA_ERR = 1 << 5;
  const EXTRA_CHG_STATE = 1 << 6;
  const EXTRA_FAN = 1 << 7;
  const RUNTIME_AC_IN = 1 << 0;
  const RUNTIME_AC_OUT = 1 << 1;
  const RUNTIME_BMS = 1 << 2;
  const RUNTIME_CELL_VOLTS = 1 << 3;
  const RUNTIME_CAPACITY = 1 << 4;
  const RUNTIME_PCS_TEMP = 1 << 5;
  const RUNTIME_CHG_LIMIT = 1 << 6;
  const RUNTIME_DSG_LIMIT = 1 << 7;

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
      ecoflow: {},
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

    // v6: EcoFlow power-station bridge block + S3 ESP health
    const ECOFLOW_BLOCK_BYTES = 59;
    if (offset + ECOFLOW_BLOCK_BYTES > bytes.length) {
      throw new Error("Truncated telemetry EcoFlow trailer");
    }
    const ecoFlags = u8(view, offset); offset += 1;
    data.ecoflow.linkFresh = (ecoFlags & 0x01) !== 0;
    data.ecoflow.bleConnected = (ecoFlags & 0x02) !== 0;
    data.ecoflow.valid = (ecoFlags & 0x04) !== 0;
    data.ecoflow.acOutputOn = (ecoFlags & 0x08) !== 0;
    data.ecoflow.dcOutputOn = (ecoFlags & 0x10) !== 0;
    data.ecoflow.espNowEncrypted = (ecoFlags & 0x20) !== 0;
    data.ecoflow.bridgeEspValid = (ecoFlags & 0x40) !== 0;
    data.ecoflow.usbOutputOn = (ecoFlags & 0x80) !== 0;
    data.gateway.ecoflowLink = data.ecoflow.linkFresh;
    data.ecoflow.soc = u8(view, offset); offset += 1;
    data.ecoflow.haveTemperature = u8(view, offset) !== 0; offset += 1;
    data.ecoflow.acOutputW = i16(view, offset); offset += 2;
    data.ecoflow.acInputW = i16(view, offset); offset += 2;
    data.ecoflow.dcOutputW = i16(view, offset); offset += 2;
    data.ecoflow.solarInputW = i16(view, offset); offset += 2;
    data.ecoflow.usbOutputW = i16(view, offset); offset += 2;
    const tempRaw = i16(view, offset); offset += 2;
    data.ecoflow.tempC = data.ecoflow.haveTemperature ? tempRaw / 10 : null;
    data.ecoflow.remainMinutes = u16(view, offset); offset += 2;
    data.ecoflow.seq = u32(view, offset); offset += 4;
    data.ecoflow.telemetryCount = u32(view, offset); offset += 4;
    data.ecoflow.error = readFixedString(bytes, offset, 24); offset += 24;
    data.ecoflowBridgeEsp = {
      cpu0: u8(view, offset),
      cpu1: u8(view, offset + 1),
      chipTemp: i16(view, offset + 2) / 10,
      heapKb: u16(view, offset + 4),
      uptimeSec: u32(view, offset + 6),
      valid: data.ecoflow.bridgeEspValid
    };
    offset += 10;

    // v9: station detail, 25 bytes. extraFlags match the ESP-NOW extra bits.
    const ECOFLOW_DETAIL_BYTES = 25;
    if (offset + ECOFLOW_DETAIL_BYTES > bytes.length) {
      throw new Error("Truncated telemetry EcoFlow detail");
    }
    const extra = u8(view, offset); offset += 1;
    data.ecoflow.acInputOn = (extra & EXTRA_AC_INPUT_ON) !== 0;
    data.ecoflow.pvInputOn = (extra & EXTRA_PV_INPUT_ON) !== 0;
    data.ecoflow.haveCellTemp = (extra & EXTRA_CELL_TEMP) !== 0;
    data.ecoflow.haveSoh = (extra & EXTRA_SOH) !== 0;
    data.ecoflow.haveFreq = (extra & EXTRA_FREQ) !== 0;
    data.ecoflow.haveErr = (extra & EXTRA_ERR) !== 0;
    data.ecoflow.haveChgState = (extra & EXTRA_CHG_STATE) !== 0;
    data.ecoflow.haveFan = (extra & EXTRA_FAN) !== 0;
    data.ecoflow.soh = data.ecoflow.haveSoh ? u8(view, offset) : null; offset += 1;
    data.ecoflow.acInFreqHz = data.ecoflow.haveFreq ? u8(view, offset) : null; offset += 1;
    data.ecoflow.chgDsgState = data.ecoflow.haveChgState ? u8(view, offset) : null; offset += 1;
    data.ecoflow.fanLevel = data.ecoflow.haveFan ? u8(view, offset) : null; offset += 1;
    data.ecoflow.chargeLimitPercent = data.ecoflow.haveChargeLimit ? u8(view, offset) : null;
    offset += 1;
    data.ecoflow.dischargeLimitPercent = data.ecoflow.haveDischargeLimit ? u8(view, offset) : null;
    offset += 1;
    data.ecoflow.inSumW = i16(view, offset); offset += 2;
    data.ecoflow.outSumW = i16(view, offset); offset += 2;
    data.ecoflow.bmsPowerW = i16(view, offset); offset += 2;
    data.ecoflow.minCellTempC = data.ecoflow.haveCellTemp ? i16(view, offset) : null; offset += 2;
    data.ecoflow.maxCellTempC = data.ecoflow.haveCellTemp ? i16(view, offset) : null; offset += 2;
    data.ecoflow.minMosTempC = data.ecoflow.haveCellTemp ? i16(view, offset) : null; offset += 2;
    data.ecoflow.maxMosTempC = data.ecoflow.haveCellTemp ? i16(view, offset) : null; offset += 2;
    data.ecoflow.errCode = data.ecoflow.haveErr ? u32(view, offset) : null; offset += 4;

    const ECOFLOW_RUNTIME_BYTES = 25;
    if (offset + ECOFLOW_RUNTIME_BYTES > bytes.length) {
      throw new Error("Truncated telemetry EcoFlow runtime");
    }
    const runtime = u8(view, offset); offset += 1;
    data.ecoflow.haveAcInElec = (runtime & RUNTIME_AC_IN) !== 0;
    data.ecoflow.haveAcOutElec = (runtime & RUNTIME_AC_OUT) !== 0;
    data.ecoflow.haveBmsElec = (runtime & RUNTIME_BMS) !== 0;
    data.ecoflow.haveCellVolts = (runtime & RUNTIME_CELL_VOLTS) !== 0;
    data.ecoflow.haveCapacity = (runtime & RUNTIME_CAPACITY) !== 0;
    data.ecoflow.havePcsTemp = (runtime & RUNTIME_PCS_TEMP) !== 0;
    data.ecoflow.haveChargeLimit = (runtime & RUNTIME_CHG_LIMIT) !== 0;
    data.ecoflow.haveDischargeLimit = (runtime & RUNTIME_DSG_LIMIT) !== 0;
    data.ecoflow.acInVolts = data.ecoflow.haveAcInElec ? i16(view, offset) / VOLT_DECI : null; offset += 2;
    data.ecoflow.acOutVolts = data.ecoflow.haveAcOutElec ? i16(view, offset) / VOLT_DECI : null; offset += 2;
    data.ecoflow.acInAmps = data.ecoflow.haveAcInElec ? i16(view, offset) / AMP_CENTI : null; offset += 2;
    data.ecoflow.acOutAmps = data.ecoflow.haveAcOutElec ? i16(view, offset) / AMP_CENTI : null; offset += 2;
    data.ecoflow.bmsVolts = data.ecoflow.haveBmsElec ? i16(view, offset) / AMP_CENTI : null; offset += 2;
    data.ecoflow.bmsAmps = data.ecoflow.haveBmsElec ? i16(view, offset) / AMP_CENTI : null; offset += 2;
    data.ecoflow.minCellMv = data.ecoflow.haveCellVolts ? u16(view, offset) : null; offset += 2;
    data.ecoflow.maxCellMv = data.ecoflow.haveCellVolts ? u16(view, offset) : null; offset += 2;
    data.ecoflow.fullCapMah = data.ecoflow.haveCapacity ? u16(view, offset) : null; offset += 2;
    data.ecoflow.remainCapMah = data.ecoflow.haveCapacity ? u16(view, offset) : null; offset += 2;
    data.ecoflow.pcsDcTempC = data.ecoflow.havePcsTemp ? i16(view, offset) : null; offset += 2;
    data.ecoflow.pcsAcTempC = data.ecoflow.havePcsTemp ? i16(view, offset) : null; offset += 2;

    return data;
  }

  global.WattcycleBinary = { decodeTelemetry: decodeTelemetry };
})(window);
