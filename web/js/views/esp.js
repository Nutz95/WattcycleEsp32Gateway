(function (global) {
  function formatUptime(seconds) {
    if (seconds == null) return "--";
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return h + "h " + m + "m " + s + "s";
  }

  function renderEsp(data) {
    const esp = data.esp || {};
    document.getElementById("cpu0").textContent =
      esp.cpu0 == null ? "--%" : esp.cpu0 + "%";
    document.getElementById("cpu1").textContent =
      esp.cpu1 == null ? "--%" : esp.cpu1 + "%";
    document.getElementById("chipTemp").textContent =
      esp.chipTemp == null || Number.isNaN(esp.chipTemp)
        ? "-- °C"
        : esp.chipTemp.toFixed(1) + " °C";
    document.getElementById("heap").textContent =
      esp.heapKb == null ? "-- KB" : esp.heapKb + " KB";
    document.getElementById("uptime").textContent = formatUptime(esp.uptimeSec);
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.esp = renderEsp;
})(window);
