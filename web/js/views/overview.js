(function (global) {
  let lastTotalAh = 0;

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

  function clearChartCanvases() {
    const empty = [{ points: [] }];
    global.WattcycleCharts.drawSeries(document.getElementById("chartVoltage"), empty);
    global.WattcycleCharts.drawSeries(document.getElementById("chartCurrent"), empty);
    global.WattcycleCharts.drawSeries(document.getElementById("chartPower"), empty);
    global.WattcycleCharts.drawSeries(document.getElementById("chartCapacity"), empty);
    const temps = document.getElementById("chartTemps");
    if (temps) {
      global.WattcycleCharts.drawSeries(temps, [{ points: [] }, { points: [] }]);
    }
  }

  function bindClearButton() {
    const button = document.getElementById("clearHistoryBtn");
    if (!button || button.dataset.bound === "1") {
      return;
    }
    button.dataset.bound = "1";
    button.addEventListener("click", function () {
      global.WattcycleHistory.clear();
      clearChartCanvases();
    });
  }

  function renderOverview(data, history) {
    bindClearButton();
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

    if (data.valid && typeof data.totalAh === "number" && data.totalAh > 0) {
      lastTotalAh = data.totalAh;
    }

    global.WattcycleCharts.drawSeries(
      document.getElementById("chartVoltage"),
      [{ name: "V", color: "#60a5fa", points: toPoints(history, "v") }],
      { unit: "V", yDigits: 2 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartCurrent"),
      [{ name: "I", points: toPoints(history, "i") }],
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
      document.getElementById("chartPower"),
      [{ name: "P", points: toPoints(history, "p") }],
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
    if (lastTotalAh > 0) {
      capacityGuides.push(
        { v: lastTotalAh * 0.1, label: "10%", color: "rgba(255,107,107,0.7)" },
        { v: lastTotalAh * 0.9, label: "90%", color: "rgba(240,180,41,0.7)" }
      );
    }
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartCapacity"),
      [{ name: "Ah", color: "#c084fc", points: toPoints(history, "ah") }],
      { unit: "Ah", yDigits: 1, guides: capacityGuides }
    );
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.overview = renderOverview;
})(window);
