(function () {
  function activateTab(name) {
    document.querySelectorAll(".tab").forEach(function (tab) {
      tab.classList.toggle("active", tab.getAttribute("data-tab") === name);
    });
    document.querySelectorAll(".view").forEach(function (view) {
      view.hidden = view.getAttribute("data-view") !== name;
    });
  }

  document.querySelectorAll(".tab").forEach(function (tab) {
    tab.addEventListener("click", function () {
      activateTab(tab.getAttribute("data-tab"));
    });
  });

  async function refresh() {
    try {
      const response = await fetch("/api/telemetry.bin", { cache: "no-store" });
      if (!response.ok) {
        throw new Error("HTTP " + response.status);
      }
      const buffer = await response.arrayBuffer();
      const data = window.WattcycleBinary.decodeTelemetry(buffer);
      const history = data.valid
        ? window.WattcycleHistory.pushSample({
            t: Date.now(),
            v: data.voltage,
            i: data.current,
            p: data.power,
            ah: data.remainingAh,
            mos: data.mosTemp,
            pcb: data.pcbTemp
          })
        : window.WattcycleHistory.all();
      window.WattcycleViews.overview(data, history);
      window.WattcycleViews.cells(data);
      window.WattcycleViews.temperatures(data, history);
      window.WattcycleViews.warnings(data);
      window.WattcycleViews.gateway(data);
      window.WattcycleViews.esp(data);
    } catch (error) {
      const summary = document.getElementById("summary");
      if (summary) {
        summary.textContent = "API unreachable: " + error.message;
      }
    }
  }

  activateTab("overview");
  refresh();
  // Match BMS poll cadence (~2s); ESP/CPU still updates on the same payload.
  setInterval(refresh, 2000);
})();
