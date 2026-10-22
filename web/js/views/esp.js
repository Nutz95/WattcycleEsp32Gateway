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

  function setLabel(id, text) {
    const el = document.getElementById(id);
    if (el) el.textContent = text;
  }

  function ensureHubRole() {
    if ((global.WattcycleDevice && global.WattcycleDevice.role) || global.WattcycleDeviceFetch) {
      return;
    }
    global.WattcycleDeviceFetch = true;
    fetch("/api/device", { credentials: "same-origin" })
      .then(function (res) {
        if (!res.ok) {
          global.WattcycleDeviceFetch = false;
          return null;
        }
        return res.json();
      })
      .then(function (body) {
        if (body) global.WattcycleDevice = body;
      })
      .catch(function () {
        global.WattcycleDeviceFetch = false;
      });
  }

  function applyHubEsp(esp) {
    ensureHubRole();
    const role = global.WattcycleDevice && global.WattcycleDevice.role;
    const p4 = role === "p4_hub";
    setLabel("hubEspTitle", p4 ? "ESP32-P4 hub" : role === "m5_hub" ? "M5Stack hub" : "This hub");
    setLabel("cpu0Label", p4 ? "P4 core 0" : "CPU core 0");
    setLabel("cpu1Label", p4 ? "P4 core 1" : "CPU core 1");
    setLabel("chipTempLabel", p4 ? "P4 temp" : "Chip temp");
    setLabel("heapLabel", p4 ? "P4 heap" : "Free heap");
    setLabel("uptimeLabel", p4 ? "P4 uptime" : "Uptime");
    const row = document.getElementById("c6HeapRow");
    if (row) row.hidden = !p4;
    const c6 = document.getElementById("c6Heap");
    if (c6) {
      const kb = esp && esp.c6HeapKb;
      c6.textContent = kb == null ? "-- KB" : kb + " KB";
    }
  }

  function renderEsp(data) {
    applyHubEsp(data.esp || {});
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

    const ecoBridge = data.ecoflowBridgeEsp || {};
    const ecoOk = !!ecoBridge.valid;
    const ecoHint = document.getElementById("ecoflowBridgeEspHint");
    if (ecoHint) {
      ecoHint.textContent = ecoOk
        ? "Live from EcoFlow ESP-NOW packet"
        : "Waiting for ESP-NOW EcoFlow bridge health…";
    }
    fillEsp(
      {
        cpu0: document.getElementById("ecoflowBridgeCpu0"),
        cpu1: document.getElementById("ecoflowBridgeCpu1"),
        chipTemp: document.getElementById("ecoflowBridgeChipTemp"),
        heap: document.getElementById("ecoflowBridgeHeap"),
        uptime: document.getElementById("ecoflowBridgeUptime"),
      },
      ecoBridge,
      ecoOk
    );
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.esp = renderEsp;
})(window);
