(function (global) {
  function fmt(value, suffix, digits) {
    if (value === undefined || value === null || Number.isNaN(value)) {
      return "--" + (suffix ? " " + suffix : "");
    }
    return Number(value).toFixed(digits) + (suffix ? " " + suffix : "");
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
    setLink("ecoflowPageBle", !!eco.bleConnected, eco.bleConnected ? "up" : "down");

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
    setLink("ecoflowPageAcOn", !!eco.acOutputOn, eco.acOutputOn ? "on" : "off");
    setLink("ecoflowPageDcOn", !!eco.dcOutputOn, eco.dcOutputOn ? "on" : "off");
    setLink("ecoflowPageUsbOn", !!eco.usbOutputOn, eco.usbOutputOn ? "on" : "off");
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.ecoflow = renderEcoflow;
})(window);
