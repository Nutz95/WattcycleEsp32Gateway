(function (global) {
  function fmt(value) {
    return value == null || Number.isNaN(value) ? "-- °C" : value.toFixed(1) + " °C";
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

    global.WattcycleCharts.drawSeries(document.getElementById("chartTemps"), [
      { color: "#fb7185", points: toPoints(history, "mos") },
      { color: "#38bdf8", points: toPoints(history, "pcb") }
    ], { legend: "MOS / PCB" });
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.temperatures = renderTemperatures;
})(window);
