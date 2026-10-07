(function (global) {
  function fmt(value) {
    return value == null || Number.isNaN(value) ? "-- °C" : value.toFixed(1) + " °C";
  }

  function renderTemperatures(data, history) {
    document.getElementById("mosTemp").textContent = fmt(data.mosTemp);
    document.getElementById("pcbTemp").textContent = fmt(data.pcbTemp);
    const root = document.getElementById("cellTemps");
    root.innerHTML = "";
    (data.cellTemps || []).forEach(function (temp, index) {
      const div = document.createElement("div");
      div.className = "cell";
      div.innerHTML = "<small>T" + (index + 1) + "</small>" + temp.toFixed(1) + " °C";
      root.appendChild(div);
    });

    const points = history || [];
    global.WattcycleCharts.drawSeries(document.getElementById("chartTemps"), [
      { color: "#fb7185", values: points.map(function (p) { return p.mos; }) },
      { color: "#38bdf8", values: points.map(function (p) { return p.pcb; }) }
    ], { legend: "MOS (pink) / PCB (blue)" });
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.temperatures = renderTemperatures;
})(window);
