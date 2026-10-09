(function (global) {
  function fmt(value, suffix, digits) {
    if (value === undefined || value === null || Number.isNaN(value)) {
      return "--" + (suffix ? " " + suffix : "");
    }
    return Number(value).toFixed(digits) + (suffix ? " " + suffix : "");
  }

  function renderDashboard(data) {
    document.getElementById("soc").textContent = data.valid ? data.soc + "%" : "--%";
    document.getElementById("summary").textContent = data.valid
      ? "Live telemetry from Wattcycle BMS"
      : "No valid BMS snapshot yet";
    document.getElementById("voltage").textContent = fmt(data.voltage, "V", 2);
    document.getElementById("current").textContent = fmt(data.current, "A", 2);
    document.getElementById("power").textContent = fmt(data.power, "W", 0);
    document.getElementById("capacity").textContent =
      fmt(data.remainingAh, "", 1) + " / " + fmt(data.totalAh, "Ah", 1);
    document.getElementById("cycles").textContent = data.cycles ?? "--";
    // SOH = State of Health. Many Wattcycle frames omit it (stays 0) — show "--" then.
    const soh = data.soh;
    document.getElementById("soh").textContent =
      soh != null && soh > 0 ? soh + "%" : "--%";

    const solar = data.solar || {};
    const solarLink = !!(data.gateway && data.gateway.solarLink);
    const sppOk = !!(data.gateway && data.gateway.spp);
    document.getElementById("solarSummary").textContent = solarLink
      ? (solar.valid ? "Live solar wattmeter — open Solar tab for charts / resets"
                     : "Bridge up — waiting for meter frames")
      : "No ESP-NOW packets from XT369P bridge";
    const sppEl = document.getElementById("solarSpp");
    sppEl.textContent = solarLink ? (sppOk ? "connected" : "down") : "no link";
    sppEl.className = solarLink && sppOk ? "ok" : "bad";
    document.getElementById("solarVoltage").textContent = fmt(solar.voltage, "V", 2);
    document.getElementById("solarCurrent").textContent = fmt(solar.current, "A", 3);
    document.getElementById("solarPower").textContent = fmt(solar.power, "W", 2);
    document.getElementById("solarEnergy").textContent = fmt(solar.energyWh, "Wh", 3);
    document.getElementById("solarCapacity").textContent = fmt(solar.capacityAh, "Ah", 3);

    const eco = data.ecoflow || {};
    const ecoLink = !!eco.linkFresh || !!(data.gateway && data.gateway.ecoflowLink);
    const ecoSummary = document.getElementById("ecoflowSummary");
    if (ecoSummary) {
      ecoSummary.textContent = ecoLink
        ? (eco.valid ? "Live station — open EcoFlow tab for detail"
                     : "Bridge up — waiting for station frames")
        : "No ESP-NOW packets from EcoFlow bridge";
    }
    const ecoSoc = document.getElementById("ecoflowSoc");
    if (ecoSoc) {
      ecoSoc.textContent = eco.valid && eco.soc != null ? eco.soc + "%" : "--%";
    }
    const ecoAc = document.getElementById("ecoflowAcOut");
    if (ecoAc) ecoAc.textContent = fmt(eco.acOutputW, "W", 0);
    const ecoSolar = document.getElementById("ecoflowSolar");
    if (ecoSolar) ecoSolar.textContent = fmt(eco.solarInputW, "W", 0);
    const ecoBle = document.getElementById("ecoflowBle");
    if (ecoBle) {
      ecoBle.textContent = ecoLink ? (eco.bleConnected ? "up" : "down") : "no link";
      ecoBle.className = ecoLink && eco.bleConnected ? "ok" : "bad";
    }

    const ntpEl = document.getElementById("overviewNtp");
    const wifiEl = document.getElementById("overviewWifi");
    if (ntpEl) {
      const ntpOk = !!(data.gateway && data.gateway.ntp);
      ntpEl.textContent = data.gateway ? (ntpOk ? "synced" : "pending") : "--";
      ntpEl.className = ntpOk ? "ok" : "bad";
    }
    if (wifiEl) {
      wifiEl.textContent = data.gateway && data.gateway.wifi ? (data.gateway.ip || "ok") : "down";
      wifiEl.className = data.gateway && data.gateway.wifi ? "ok" : "bad";
    }
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.dashboard = renderDashboard;
})(window);
