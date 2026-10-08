(function (global) {
  function formatUptime(seconds) {
    if (seconds == null) return "--";
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = seconds % 60;
    return h + "h " + m + "m " + s + "s";
  }

  function fillEsp(ids, esp, valid) {
    if (!valid || !esp) {
      ids.cpu0.textContent = "--%";
      ids.cpu1.textContent = "--%";
      ids.chipTemp.textContent = "-- °C";
      ids.heap.textContent = "-- KB";
      ids.uptime.textContent = "--";
      return;
    }
    ids.cpu0.textContent = esp.cpu0 == null ? "--%" : esp.cpu0 + "%";
    ids.cpu1.textContent = esp.cpu1 == null ? "--%" : esp.cpu1 + "%";
    ids.chipTemp.textContent =
      esp.chipTemp == null || Number.isNaN(esp.chipTemp)
        ? "-- °C"
        : esp.chipTemp.toFixed(1) + " °C";
    ids.heap.textContent = esp.heapKb == null ? "-- KB" : esp.heapKb + " KB";
    ids.uptime.textContent = formatUptime(esp.uptimeSec);
  }

  function renderEsp(data) {
    fillEsp(
      {
        cpu0: document.getElementById("cpu0"),
        cpu1: document.getElementById("cpu1"),
        chipTemp: document.getElementById("chipTemp"),
        heap: document.getElementById("heap"),
        uptime: document.getElementById("uptime"),
      },
      data.esp || {},
      true
    );

    const bmsBridge = data.bmsBridgeEsp || {};
    const bmsOk = !!bmsBridge.valid;
    const bmsHint = document.getElementById("bmsBridgeEspHint");
    if (bmsHint) {
      bmsHint.textContent = bmsOk
        ? "Live from Wattcycle ESP-NOW telemetry"
        : "Waiting for ESP-NOW BMS bridge health…";
    }
    fillEsp(
      {
        cpu0: document.getElementById("bmsBridgeCpu0"),
        cpu1: document.getElementById("bmsBridgeCpu1"),
        chipTemp: document.getElementById("bmsBridgeChipTemp"),
        heap: document.getElementById("bmsBridgeHeap"),
        uptime: document.getElementById("bmsBridgeUptime"),
      },
      bmsBridge,
      bmsOk
    );

    const bridge = data.bridgeEsp || {};
    const bridgeOk = !!bridge.valid;
    const hint = document.getElementById("bridgeEspHint");
    if (hint) {
      hint.textContent = bridgeOk
        ? "Live from XT369P ESP-NOW packet"
        : "Waiting for ESP-NOW XT bridge health…";
    }
    fillEsp(
      {
        cpu0: document.getElementById("bridgeCpu0"),
        cpu1: document.getElementById("bridgeCpu1"),
        chipTemp: document.getElementById("bridgeChipTemp"),
        heap: document.getElementById("bridgeHeap"),
        uptime: document.getElementById("bridgeUptime"),
      },
      bridge,
      bridgeOk
    );
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.esp = renderEsp;
})(window);
