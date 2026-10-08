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

  async function sendCommand(command) {
    const status = document.getElementById("solarCmdStatus");
    if (status) status.textContent = "Sending…";
    try {
      const response = await fetch("/api/solar/command", {
        method: "POST",
        credentials: "same-origin",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ command: command })
      });
      const body = await response.json().catch(function () { return {}; });
      if (!response.ok) {
        throw new Error(body.error || ("HTTP " + response.status));
      }
      if (status) status.textContent = "Sent " + command;
    } catch (error) {
      if (status) status.textContent = "Failed: " + error.message;
    }
  }

  function bindCommands() {
    document.querySelectorAll("[data-solar-cmd]").forEach(function (button) {
      if (button.dataset.bound === "1") return;
      button.dataset.bound = "1";
      button.addEventListener("click", function () {
        sendCommand(button.getAttribute("data-solar-cmd"));
      });
    });
  }

  function renderSolar(data, solarHistory) {
    bindCommands();
    const solar = data.solar || {};
    const solarLink = !!(data.gateway && data.gateway.solarLink);
    const sppOk = !!(data.gateway && data.gateway.spp);

    document.getElementById("solarPagePower").textContent = solar.valid
      ? fmt(solar.power, "W", 2)
      : "-- W";
    document.getElementById("solarPageSummary").textContent = solarLink
      ? (solar.valid ? "Live XT369P via ESP-NOW" : "Bridge up — waiting for meter")
      : "No ESP-NOW link";
    setLink("solarPageLink", solarLink, solarLink ? "fresh" : "stale");
    setLink("solarPageSpp", sppOk, sppOk ? "connected" : "down");
    const encrypted = !!solar.espNowEncrypted;
    const encEl = document.getElementById("solarPageEnc");
    if (encEl) {
      encEl.textContent = solarLink || encrypted
        ? (encrypted ? "secured" : "plaintext")
        : "--";
      encEl.className = encrypted ? "ok" : (solarLink ? "warn" : "bad");
    }
    document.getElementById("solarPageVoltage").textContent = fmt(solar.voltage, "V", 2);
    document.getElementById("solarPageCurrent").textContent = fmt(solar.current, "A", 3);
    document.getElementById("solarPagePowerMetric").textContent = fmt(solar.power, "W", 2);
    document.getElementById("solarPageEnergy").textContent = fmt(solar.energyWh, "Wh", 3);
    document.getElementById("solarPageCapacity").textContent = fmt(solar.capacityAh, "Ah", 3);
    document.getElementById("solarPageTemp").textContent = fmt(solar.tempC, "°C", 1);

    global.WattcycleCharts.drawSeries(
      document.getElementById("chartSolarVoltage"),
      [{ name: "V", color: "#60a5fa", points: global.WattcycleSolarHistory.toPoints(solarHistory, "v") }],
      { unit: "V", yDigits: 2 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartSolarCurrent"),
      [{ name: "I", color: "#3ecf8e", points: global.WattcycleSolarHistory.toPoints(solarHistory, "i") }],
      { unit: "A", yDigits: 3 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartSolarPower"),
      [{ name: "P", color: "#f0b429", points: global.WattcycleSolarHistory.toPoints(solarHistory, "p") }],
      { unit: "W", yDigits: 2 }
    );
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.solar = renderSolar;
})(window);
