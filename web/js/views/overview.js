(function (global) {
  function fmt(value, suffix, digits) {
    if (value === undefined || value === null || Number.isNaN(value)) {
      return "--" + (suffix ? " " + suffix : "");
    }
    return Number(value).toFixed(digits) + (suffix ? " " + suffix : "");
  }

  function toPoints(history, key) {
    return (history || [])
      .filter(function (sample) {
        return typeof sample.t === "number" && typeof sample[key] === "number";
      })
      .map(function (sample) {
        return { t: sample.t, v: sample[key] };
      });
  }

  function renderOverview(data, history) {
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

    global.WattcycleCharts.drawSeries(document.getElementById("chartVoltage"), [
      { color: "#60a5fa", points: toPoints(history, "v") }
    ]);
    global.WattcycleCharts.drawSeries(document.getElementById("chartCurrent"), [
      { color: "#3ecf8e", points: toPoints(history, "i") }
    ]);
    global.WattcycleCharts.drawSeries(document.getElementById("chartPower"), [
      { color: "#f0b429", points: toPoints(history, "p") }
    ]);
    global.WattcycleCharts.drawSeries(document.getElementById("chartCapacity"), [
      { color: "#c084fc", points: toPoints(history, "ah") }
    ]);
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.overview = renderOverview;
})(window);
