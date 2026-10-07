(function (global) {
  function renderCells(data) {
    const root = document.getElementById("cells");
    root.innerHTML = "";
    (data.cells || []).forEach(function (voltage, index) {
      const div = document.createElement("div");
      div.className = "cell";
      div.innerHTML = "<small>C" + (index + 1) + "</small>" + voltage.toFixed(3) + " V";
      root.appendChild(div);
    });
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.cells = renderCells;
})(window);
