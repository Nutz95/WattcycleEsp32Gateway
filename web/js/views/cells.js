(function (global) {
  function renderCells(data) {
    const cells = data.cells || [];
    const balancing = data.balancing || [];
    const root = document.getElementById("cells");
    root.innerHTML = "";
    if (!cells.length) {
      document.getElementById("cellMax").textContent = "-- V";
      document.getElementById("cellMin").textContent = "-- V";
      document.getElementById("cellDelta").textContent = "-- mV";
      document.getElementById("cellAvg").textContent = "-- V";
      document.getElementById("balanceCurrent").textContent = "-- A";
      return;
    }

    let min = cells[0];
    let max = cells[0];
    let sum = 0;
    cells.forEach(function (voltage) {
      sum += voltage;
      if (voltage < min) min = voltage;
      if (voltage > max) max = voltage;
    });
    document.getElementById("cellMax").textContent = max.toFixed(3) + " V";
    document.getElementById("cellMin").textContent = min.toFixed(3) + " V";
    document.getElementById("cellDelta").textContent =
      ((max - min) * 1000).toFixed(0) + " mV";
    document.getElementById("cellAvg").textContent =
      (sum / cells.length).toFixed(3) + " V";
    const balanceCurrent = data.balanceCurrent;
    document.getElementById("balanceCurrent").textContent =
      balanceCurrent == null || Number.isNaN(balanceCurrent)
        ? "-- A"
        : balanceCurrent.toFixed(2) + " A";

    cells.forEach(function (voltage, index) {
      const div = document.createElement("div");
      div.className = "cell";
      if (balancing[index]) div.className += " cell-balancing";
      if (voltage >= max - 0.0001) div.className += " cell-max";
      if (voltage <= min + 0.0001) div.className += " cell-min";
      div.innerHTML =
        "<small>C" + (index + 1) + (balancing[index] ? " bal" : "") + "</small>" +
        voltage.toFixed(3) + " V";
      root.appendChild(div);
    });
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.cells = renderCells;
})(window);
