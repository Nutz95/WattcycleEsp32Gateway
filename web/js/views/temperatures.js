(function (global) {
  function fmt(value) {
    return value == null || Number.isNaN(value) ? "-- °C" : value.toFixed(1) + " °C";
  }

  function renderTemperatures(data) {
    document.getElementById("mosTemp").textContent = fmt(data.mosTemp);
    document.getElementById("pcbTemp").textContent = fmt(data.pcbTemp);
    const root = document.getElementById("cellTemps");
    const hint = document.getElementById("cellTempsHint");
    root.innerHTML = "";
    const temps = data.cellTemps || [];
    if (hint) {
      hint.textContent = temps.length
        ? temps.length + " cell sensor(s) from BMS bridge"
        : "No cell sensors in this snapshot yet (need ESP-NOW v2 + bridge flash).";
    }
    temps.forEach(function (temp, index) {
      const div = document.createElement("div");
      div.className = "cell";
      div.innerHTML = "<small>T" + (index + 1) + "</small>" + temp.toFixed(1) + " °C";
      root.appendChild(div);
    });
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.temperatures = renderTemperatures;
})(window);
