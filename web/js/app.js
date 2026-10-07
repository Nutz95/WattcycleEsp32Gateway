(function () {
  let refreshTimer = null;

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
      const response = await fetch("/api/telemetry.bin", {
        cache: "no-store",
        credentials: "same-origin"
      });
      if (response.status === 401) {
        if (refreshTimer) {
          clearInterval(refreshTimer);
          refreshTimer = null;
        }
        window.WattcycleAuth.handleUnauthorized();
        return;
      }
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
      window.WattcycleViews.account();
    } catch (error) {
      const summary = document.getElementById("summary");
      if (summary) {
        summary.textContent = "API unreachable: " + error.message;
      }
    }
  }

  function startDashboard() {
    window.WattcycleAuth.showApp();
    activateTab("overview");
    refresh();
    if (refreshTimer) {
      clearInterval(refreshTimer);
    }
    refreshTimer = setInterval(refresh, 2000);
  }

  window.WattcycleAuth.bootstrap(startDashboard);
})();
