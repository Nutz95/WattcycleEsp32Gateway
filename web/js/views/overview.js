(function (global) {
  function fmt(value, suffix, digits) {
    if (value === undefined || value === null || Number.isNaN(value)) {
      return "--" + (suffix ? " " + suffix : "");
    }
    return Number(value).toFixed(digits) + (suffix ? " " + suffix : "");
  }

  function renderOverview(data) {
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
    document.getElementById("soh").textContent = data.soh != null ? data.soh + "%" : "--%";
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.overview = renderOverview;
})(window);
