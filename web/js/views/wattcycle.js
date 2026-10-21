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

  function renderWattcycle(data, packHistory) {
    const gw = data.gateway || {};
    // Fresh = recent ESP-NOW from BMS bridge (not last battery.valid latch).
    const link = !!gw.telemetryFresh;
    const bleUp = link && !!gw.ble;

    document.getElementById("wattcyclePageSoc").textContent = data.valid
      ? data.soc + "%"
      : "--%";
    document.getElementById("wattcyclePageSummary").textContent = link
      ? (data.valid ? "Live Wattcycle BMS via ESP-NOW" : "Bridge up — waiting for BMS frames")
      : "No ESP-NOW link from BMS bridge";
    setLink("wattcyclePageLink", link, link ? "fresh" : "stale");
    setLink("wattcyclePageBle", bleUp, link ? (gw.ble ? "up" : "down") : "no link");

    document.getElementById("wattcyclePageVoltage").textContent = fmt(data.voltage, "V", 2);
    document.getElementById("wattcyclePageCurrent").textContent = fmt(data.current, "A", 2);
    document.getElementById("wattcyclePagePower").textContent = fmt(data.power, "W", 0);
    document.getElementById("wattcyclePageCapacity").textContent =
      fmt(data.remainingAh, "", 1) + " / " + fmt(data.totalAh, "Ah", 1);
    document.getElementById("wattcyclePageCycles").textContent =
      data.cycles != null ? String(data.cycles) : "--";
    const soh = data.soh;
    document.getElementById("wattcyclePageSoh").textContent =
      soh != null && soh > 0 ? soh + "%" : "--%";
    document.getElementById("wattcyclePageMos").textContent = fmt(data.mosTemp, "°C", 1);
    document.getElementById("wattcyclePagePcb").textContent = fmt(data.pcbTemp, "°C", 1);

    const hist = packHistory || global.WattcycleHistory.all();
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartWcVoltage"),
      [{ name: "V", color: "#60a5fa", points: global.WattcycleHistory.toPoints(hist, "v") }],
      { unit: "V", yDigits: 2 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartWcCurrent"),
      [{ name: "I", points: global.WattcycleHistory.toPoints(hist, "i") }],
      {
        unit: "A",
        yDigits: 2,
        signed: true,
        legend: "+ charge / − discharge",
        positiveColor: "#3ecf8e",
        negativeColor: "#ff6b6b"
      }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartWcPower"),
      [{ name: "P", points: global.WattcycleHistory.toPoints(hist, "p") }],
      {
        unit: "W",
        yDigits: 0,
        signed: true,
        legend: "+ charge / − discharge",
        positiveColor: "#f0b429",
        negativeColor: "#ff6b6b"
      }
    );
    const capacityGuides = [];
    if (typeof data.totalAh === "number" && data.totalAh > 0) {
      capacityGuides.push(
        { v: data.totalAh * 0.1, label: "10%", color: "rgba(255,107,107,0.7)" },
        { v: data.totalAh * 0.9, label: "90%", color: "rgba(240,180,41,0.7)" }
      );
    }
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartWcCapacity"),
      [{ name: "Ah", color: "#c084fc", points: global.WattcycleHistory.toPoints(hist, "ah") }],
      { unit: "Ah", yDigits: 1, guides: capacityGuides }
    );
    global.WattcycleCharts.drawSeries(document.getElementById("chartWcTemps"), [
      { name: "MOS", color: "#fb7185", points: global.WattcycleHistory.toPoints(hist, "mos") },
      { name: "PCB", color: "#38bdf8", points: global.WattcycleHistory.toPoints(hist, "pcb") }
    ], { legend: "MOS / PCB", unit: "°C", yDigits: 1 });
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.wattcycle = renderWattcycle;
})(window);
