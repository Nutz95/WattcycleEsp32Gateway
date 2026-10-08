(function (global) {
  function formatUptime(seconds) {
    if (seconds == null) return "--";
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return h + "h " + m + "m " + s + "s";
  }

  function fillEsp(prefix, esp, valid) {
    const cpu0 = document.getElementById(prefix === "hub" ? "cpu0" : "bridgeCpu0");
    const cpu1 = document.getElementById(prefix === "hub" ? "cpu1" : "bridgeCpu1");
    const chipTemp = document.getElementById(prefix === "hub" ? "chipTemp" : "bridgeChipTemp");
    const heap = document.getElementById(prefix === "hub" ? "heap" : "bridgeHeap");
    const uptime = document.getElementById(prefix === "hub" ? "uptime" : "bridgeUptime");
    if (!valid || !esp) {
      cpu0.textContent = "--%";
      cpu1.textContent = "--%";
      chipTemp.textContent = "-- °C";
      heap.textContent = "-- KB";
      uptime.textContent = "--";
      return;
    }
    cpu0.textContent = esp.cpu0 == null ? "--%" : esp.cpu0 + "%";
    cpu1.textContent = esp.cpu1 == null ? "--%" : esp.cpu1 + "%";
    chipTemp.textContent =
      esp.chipTemp == null || Number.isNaN(esp.chipTemp)
        ? "-- °C"
        : esp.chipTemp.toFixed(1) + " °C";
    heap.textContent = esp.heapKb == null ? "-- KB" : esp.heapKb + " KB";
    uptime.textContent = formatUptime(esp.uptimeSec);
  }

  function renderEsp(data) {
    fillEsp("hub", data.esp || {}, true);
    const bridge = data.bridgeEsp || {};
    const bridgeOk = !!bridge.valid;
    const hint = document.getElementById("bridgeEspHint");
    if (hint) {
      hint.textContent = bridgeOk
        ? "Live from ESP-NOW bridge packet"
        : "Waiting for ESP-NOW bridge health…";
    }
    fillEsp("bridge", bridge, bridgeOk);
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.esp = renderEsp;
})(window);
