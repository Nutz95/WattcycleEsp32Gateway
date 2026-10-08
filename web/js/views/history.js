(function (global) {
  let lastTotalAh = 0;
  let bound = false;

  function activateHistoryTab(name) {
    document.querySelectorAll(".subtab").forEach(function (tab) {
      tab.classList.toggle("active", tab.getAttribute("data-history-tab") === name);
    });
    document.querySelectorAll(".history-panel").forEach(function (panel) {
      panel.hidden = panel.getAttribute("data-history-panel") !== name;
    });
  }

  function bind() {
    if (bound) return;
    bound = true;
    document.querySelectorAll(".subtab").forEach(function (tab) {
      tab.addEventListener("click", function () {
        activateHistoryTab(tab.getAttribute("data-history-tab"));
      });
    });
    const clearPack = document.getElementById("clearPackHistoryBtn");
    if (clearPack) {
      clearPack.addEventListener("click", function () {
        global.WattcycleHistory.clear();
      });
    }
    const clearSolar = document.getElementById("clearSolarHistoryBtn");
    if (clearSolar) {
      clearSolar.addEventListener("click", function () {
        global.WattcycleSolarHistory.clear();
      });
    }
  }

  function renderHistory(data, packHistory, solarHistory) {
    bind();
    if (data.valid && typeof data.totalAh === "number" && data.totalAh > 0) {
      lastTotalAh = data.totalAh;
    }

    global.WattcycleCharts.drawSeries(
      document.getElementById("chartVoltage"),
      [{ name: "V", color: "#60a5fa", points: global.WattcycleHistory.toPoints(packHistory, "v") }],
      { unit: "V", yDigits: 2 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartCurrent"),
      [{ name: "I", points: global.WattcycleHistory.toPoints(packHistory, "i") }],
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
      [{ name: "P", points: global.WattcycleHistory.toPoints(packHistory, "p") }],
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
      [{ name: "Ah", color: "#c084fc", points: global.WattcycleHistory.toPoints(packHistory, "ah") }],
      { unit: "Ah", yDigits: 1, guides: capacityGuides }
    );
    global.WattcycleCharts.drawSeries(document.getElementById("chartTemps"), [
      { name: "MOS", color: "#fb7185", points: global.WattcycleHistory.toPoints(packHistory, "mos") },
      { name: "PCB", color: "#38bdf8", points: global.WattcycleHistory.toPoints(packHistory, "pcb") }
    ], { legend: "MOS / PCB", unit: "°C", yDigits: 1 });

    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarVoltage"),
      [{ name: "V", color: "#60a5fa", points: global.WattcycleSolarHistory.toPoints(solarHistory, "v") }],
      { unit: "V", yDigits: 2 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarCurrent"),
      [{ name: "I", color: "#3ecf8e", points: global.WattcycleSolarHistory.toPoints(solarHistory, "i") }],
      { unit: "A", yDigits: 3 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarPower"),
      [{ name: "P", color: "#f0b429", points: global.WattcycleSolarHistory.toPoints(solarHistory, "p") }],
      { unit: "W", yDigits: 1 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarEnergy"),
      [{ name: "Wh", color: "#c084fc", points: global.WattcycleSolarHistory.toPoints(solarHistory, "wh") }],
      { unit: "Wh", yDigits: 3 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarCapacity"),
      [{ name: "Ah", color: "#38bdf8", points: global.WattcycleSolarHistory.toPoints(solarHistory, "ah") }],
      { unit: "Ah", yDigits: 3 }
    );
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.history = renderHistory;
})(window);
