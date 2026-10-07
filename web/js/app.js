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
      window.WattcycleViews.overview(data);
      window.WattcycleViews.cells(data);
      window.WattcycleViews.temperatures(data);
      window.WattcycleViews.warnings(data);
      window.WattcycleViews.gateway(data);
    } catch (error) {
      const summary = document.getElementById("summary");
      if (summary) {
        summary.textContent = "API unreachable: " + error.message;
      }
    }
  }

  activateTab("overview");
  refresh();
  setInterval(refresh, 3000);
})();
