(function (global) {
  function fmt(value, suffix, digits) {
    if (value === undefined || value === null || Number.isNaN(value)) {
      return "--" + (suffix ? " " + suffix : "");
    }
    return Number(value).toFixed(digits) + (suffix ? " " + suffix : "");
  }

  function chgLabel(state) {
    if (state == null) return "--";
    if (state === 0) return "idle";
    if (state === 1) return "discharge";
    if (state === 2) return "charge";
    return String(state);
  }

  function rangeText(min, max, suffix) {
    if (min == null || max == null) return "--";
    return min + " / " + max + (suffix ? " " + suffix : "");
  }

  function setLink(id, ok, label) {
    const el = document.getElementById(id);
    if (!el) return;
    el.textContent = label;
    el.className = ok ? "ok" : "bad";
  }

  function renderEcoflow(data) {
    const eco = data.ecoflow || {};
    const link = !!eco.linkFresh || !!(data.gateway && data.gateway.ecoflowLink);

    document.getElementById("ecoflowPageSoc").textContent = eco.valid
      ? (eco.soc != null ? eco.soc + "%" : "--%")
      : "--%";
    document.getElementById("ecoflowPageSummary").textContent = link
      ? (eco.valid ? "Live DELTA 3 via ESP-NOW" : "Bridge up — waiting for station frames")
      : "No ESP-NOW link from EcoFlow bridge";
    setLink("ecoflowPageLink", link, link ? "fresh" : "stale");
    setLink(
      "ecoflowPageBle",
      link && !!eco.bleConnected,
      link ? (eco.bleConnected ? "up" : "down") : "no link"
    );

    const encEl = document.getElementById("ecoflowPageEnc");
    if (encEl) {
      const encrypted = !!eco.espNowEncrypted;
      encEl.textContent = link || encrypted ? (encrypted ? "secured" : "plaintext") : "--";
      encEl.className = encrypted ? "ok" : (link ? "warn" : "bad");
    }

    document.getElementById("ecoflowPageAcOut").textContent = fmt(eco.acOutputW, "W", 0);
    document.getElementById("ecoflowPageAcIn").textContent = fmt(eco.acInputW, "W", 0);
    document.getElementById("ecoflowPageDcOut").textContent = fmt(eco.dcOutputW, "W", 0);
    document.getElementById("ecoflowPageSolar").textContent = fmt(eco.solarInputW, "W", 0);
    document.getElementById("ecoflowPageUsb").textContent = fmt(eco.usbOutputW, "W", 0);
    document.getElementById("ecoflowPageTemp").textContent =
      eco.haveTemperature && eco.tempC != null ? fmt(eco.tempC, "°C", 1) : "-- °C";
    document.getElementById("ecoflowPageRemain").textContent =
      eco.remainMinutes != null ? eco.remainMinutes + " min" : "-- min";
    document.getElementById("ecoflowPageSoh").textContent =
      eco.haveSoh ? eco.soh + "%" : "--";
    document.getElementById("ecoflowPageInSum").textContent = fmt(eco.inSumW, "W", 0);
    document.getElementById("ecoflowPageOutSum").textContent = fmt(eco.outSumW, "W", 0);
    const bmsEl = document.getElementById("ecoflowPageBmsW");
    if (bmsEl) {
      bmsEl.textContent = fmt(eco.bmsPowerW, "W", 0);
      bmsEl.className = Number(eco.bmsPowerW) < 0 ? "bad" : "";
    }
    document.getElementById("ecoflowPageHz").textContent =
      eco.haveFreq ? eco.acInFreqHz + " Hz" : "--";
    setLink("ecoflowPageAcInOn", !!eco.acInputOn, eco.acInputOn ? "on" : "off");
    setLink("ecoflowPagePvOn", !!eco.pvInputOn, eco.pvInputOn ? "on" : "off");
    document.getElementById("ecoflowPageState").textContent = chgLabel(eco.haveChgState ? eco.chgDsgState : null);
    document.getElementById("ecoflowPageFan").textContent = eco.haveFan ? String(eco.fanLevel) : "--";
    document.getElementById("ecoflowPageCell").textContent = eco.haveCellTemp
      ? rangeText(eco.minCellTempC, eco.maxCellTempC, "°C")
      : "--";
    document.getElementById("ecoflowPageMos").textContent = eco.haveCellTemp
      ? rangeText(eco.minMosTempC, eco.maxMosTempC, "°C")
      : "--";
    const limits = document.getElementById("ecoflowPageLimits");
    if (limits) {
      if (eco.haveDischargeLimit || eco.haveChargeLimit) {
        const low = eco.haveDischargeLimit ? eco.dischargeLimitPercent + "%" : "--";
        const high = eco.haveChargeLimit ? eco.chargeLimitPercent + "%" : "--";
        limits.textContent = low + " – " + high;
      } else {
        limits.textContent = "--";
      }
    }
    const acInVa = document.getElementById("ecoflowPageAcInVa");
    if (acInVa) {
      acInVa.textContent = eco.haveAcInElec
        ? fmt(eco.acInVolts, "V", 1) + "  " + fmt(eco.acInAmps, "A", 2)
        : "--";
    }
    const acOutVa = document.getElementById("ecoflowPageAcOutVa");
    if (acOutVa) {
      acOutVa.textContent = eco.haveAcOutElec
        ? fmt(eco.acOutVolts, "V", 1) + "  " + fmt(eco.acOutAmps, "A", 2)
        : "--";
    }
    const bmsVa = document.getElementById("ecoflowPageBmsVa");
    if (bmsVa) {
      bmsVa.textContent = eco.haveBmsElec
        ? fmt(eco.bmsVolts, "V", 2) + "  " + fmt(eco.bmsAmps, "A", 2)
        : "--";
      bmsVa.className = eco.haveBmsElec && Number(eco.bmsAmps) < 0 ? "bad" : "";
    }
    const cellMv = document.getElementById("ecoflowPageCellMv");
    if (cellMv) {
      cellMv.textContent = eco.haveCellVolts
        ? eco.minCellMv + "–" + eco.maxCellMv + " mV"
        : "--";
    }
    const capEl = document.getElementById("ecoflowPageCap");
    if (capEl) {
      capEl.textContent = eco.haveCapacity
        ? eco.remainCapMah + " / " + eco.fullCapMah + " mAh"
        : "--";
    }
    const pcsEl = document.getElementById("ecoflowPagePcs");
    if (pcsEl) {
      pcsEl.textContent = eco.havePcsTemp
        ? "DC " + eco.pcsDcTempC + "°  AC " + eco.pcsAcTempC + "°"
        : "--";
    }
    const errEl = document.getElementById("ecoflowPageErr");
    if (errEl) {
      errEl.textContent = eco.haveErr ? String(eco.errCode) : "--";
      errEl.className = eco.haveErr && eco.errCode ? "bad" : "";
    }
    setLink("ecoflowPageAcOn", !!eco.acOutputOn, eco.acOutputOn ? "on" : "off");
    setLink("ecoflowPageDcOn", !!eco.dcOutputOn, eco.dcOutputOn ? "on" : "off");
    setLink("ecoflowPageUsbOn", !!eco.usbOutputOn, eco.usbOutputOn ? "on" : "off");
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.ecoflow = renderEcoflow;
})(window);
