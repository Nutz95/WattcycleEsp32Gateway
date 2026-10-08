(function (global) {
  let lastTotalAh = 0;
  let bound = false;
  let todayMerged = false;
  let activeDay = "";
  let dayOverlayPack = null;
  let dayOverlaySolar = null;
  let loadingDay = false;

  function activateHistoryTab(name) {
    document.querySelectorAll(".subtab").forEach(function (tab) {
      tab.classList.toggle("active", tab.getAttribute("data-history-tab") === name);
    });
    document.querySelectorAll(".history-panel").forEach(function (panel) {
      panel.hidden = panel.getAttribute("data-history-panel") !== name;
    });
  }

  function utcDayString(epochMs) {
    const d = new Date(epochMs);
    const y = d.getUTCFullYear();
    const m = String(d.getUTCMonth() + 1).padStart(2, "0");
    const day = String(d.getUTCDate()).padStart(2, "0");
    return y + "-" + m + "-" + day;
  }

  function parseCsvDay(csv) {
    const pack = [];
    const solar = [];
    if (!csv) return { pack: pack, solar: solar };
    const lines = csv.split(/\r?\n/);
    for (let i = 0; i < lines.length; i += 1) {
      const line = lines[i].trim();
      if (!line || line.charAt(0) === "e" || line.charAt(0) === "E") continue;
      const cols = line.split(",");
      if (cols.length < 10) continue;
      const epoch = Number(cols[0]);
      if (!Number.isFinite(epoch) || epoch <= 0) continue;
      const t = epoch * 1000;
      const packOk = cols[8] === "1";
      const solarOk = cols[9] === "1";
      if (packOk) {
        pack.push({
          t: t,
          v: Number(cols[2]),
          i: Number(cols[3]),
          p: Number(cols[4]),
          soc: Number(cols[1])
        });
      }
      if (solarOk) {
        solar.push({
          t: t,
          v: Number(cols[5]),
          i: Number(cols[6]),
          p: Number(cols[7])
        });
      }
    }
    return { pack: pack, solar: solar };
  }

  async function refreshSdDays() {
    const statusEl = document.getElementById("historySdStatus");
    const select = document.getElementById("historyDaySelect");
    if (!statusEl || !select) return;
    try {
      const res = await fetch("/api/history/days", { credentials: "same-origin" });
      if (!res.ok) {
        statusEl.textContent = "SD history unavailable (login / hub offline).";
        return;
      }
      const body = await res.json();
      global.WattcycleDevice = Object.assign({}, global.WattcycleDevice || {}, body);
      statusEl.textContent =
        "NTP " + (body.ntp ? "synced" : "pending") + " · SD " + (body.sd ? "ready" : "missing");
      const current = select.value;
      select.innerHTML = '<option value="">Live + today (localStorage)</option>';
      (body.days || []).forEach(function (day) {
        const opt = document.createElement("option");
        opt.value = day;
        opt.textContent = day;
        select.appendChild(opt);
      });
      if (current) select.value = current;

      const today = utcDayString(Date.now());
      if (!todayMerged && body.sd && (body.days || []).indexOf(today) >= 0) {
        todayMerged = true;
        await loadDayIntoCharts(today, { mergeIntoLive: true, selectDay: false });
      }
    } catch (err) {
      statusEl.textContent = "SD history unreachable.";
    }
  }

  async function downloadDayCsv(date) {
    const statusEl = document.getElementById("historySdStatus");
    const progress = document.getElementById("historyDayProgress");
    if (!date) return null;
    if (progress) {
      progress.hidden = false;
      progress.value = 0;
    }
    if (statusEl) statusEl.textContent = "Downloading " + date + "…";
    const res = await fetch("/api/history/day?date=" + encodeURIComponent(date), {
      credentials: "same-origin",
    });
    if (!res.ok) {
      if (progress) progress.hidden = true;
      if (statusEl) statusEl.textContent = "Day download failed (" + res.status + ").";
      return null;
    }
    const total = Number(res.headers.get("Content-Length") || 0);
    if (!res.body || !res.body.getReader) {
      const text = await res.text();
      if (progress) {
        progress.value = 100;
        progress.hidden = true;
      }
      return text;
    }
    const reader = res.body.getReader();
    const chunks = [];
    let received = 0;
    for (;;) {
      const { done, value } = await reader.read();
      if (done) break;
      chunks.push(value);
      received += value.byteLength;
      if (progress && total > 0) {
        const pct = Math.min(100, Math.round((received / total) * 100));
        progress.value = pct;
        if (statusEl) {
          statusEl.textContent =
            "Downloading " + date + "… " + pct + "% (" + Math.round(received / 1024) + " KB)";
        }
      } else if (statusEl) {
        statusEl.textContent = "Downloading " + date + "… " + Math.round(received / 1024) + " KB";
      }
    }
    if (progress) {
      progress.value = 100;
      progress.hidden = true;
    }
    if (statusEl) statusEl.textContent = "Loaded " + date + " (" + Math.round(received / 1024) + " KB)";
    const merged = new Uint8Array(received);
    let offset = 0;
    chunks.forEach(function (chunk) {
      merged.set(chunk, offset);
      offset += chunk.byteLength;
    });
    return new TextDecoder().decode(merged);
  }

  async function loadDayIntoCharts(date, options) {
    const opts = options || {};
    if (!date || loadingDay) return;
    loadingDay = true;
    try {
      const csv = await downloadDayCsv(date);
      if (!csv) return;
      const parsed = parseCsvDay(csv);
      const statusEl = document.getElementById("historySdStatus");
      if (opts.mergeIntoLive) {
        global.WattcycleHistory.mergeSamples(parsed.pack);
        global.WattcycleSolarHistory.mergeSamples(parsed.solar);
        activeDay = "";
        dayOverlayPack = null;
        dayOverlaySolar = null;
        if (statusEl) {
          statusEl.textContent =
            "Merged " + date + " from SD (" + parsed.pack.length + " pack / " +
            parsed.solar.length + " solar pts) into live charts";
        }
      } else {
        activeDay = date;
        dayOverlayPack = parsed.pack;
        dayOverlaySolar = parsed.solar;
        if (opts.selectDay !== false) {
          const select = document.getElementById("historyDaySelect");
          if (select) select.value = date;
        }
        if (statusEl) {
          statusEl.textContent =
            "Showing SD day " + date + " (" + parsed.pack.length + " pack / " +
            parsed.solar.length + " solar pts)";
        }
      }
    } finally {
      loadingDay = false;
    }
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
        dayOverlayPack = null;
        activeDay = "";
      });
    }
    const clearSolar = document.getElementById("clearSolarHistoryBtn");
    if (clearSolar) {
      clearSolar.addEventListener("click", function () {
        global.WattcycleSolarHistory.clear();
        dayOverlaySolar = null;
        activeDay = "";
      });
    }
    const select = document.getElementById("historyDaySelect");
    if (select) {
      select.addEventListener("change", function () {
        if (!select.value) {
          activeDay = "";
          dayOverlayPack = null;
          dayOverlaySolar = null;
          const statusEl = document.getElementById("historySdStatus");
          if (statusEl) statusEl.textContent = "Live charts (localStorage + today SD merge)";
          return;
        }
        loadDayIntoCharts(select.value, { mergeIntoLive: false, selectDay: true });
      });
    }
    refreshSdDays();
  }

  function renderHistory(data, packHistory, solarHistory) {
    bind();
    if (data.valid && typeof data.totalAh === "number" && data.totalAh > 0) {
      lastTotalAh = data.totalAh;
    }

    const pack = activeDay && dayOverlayPack ? dayOverlayPack : packHistory;
    const solar = activeDay && dayOverlaySolar ? dayOverlaySolar : solarHistory;

    global.WattcycleCharts.drawSeries(
      document.getElementById("chartVoltage"),
      [{ name: "V", color: "#60a5fa", points: global.WattcycleHistory.toPoints(pack, "v") }],
      { unit: "V", yDigits: 2 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartCurrent"),
      [{ name: "I", points: global.WattcycleHistory.toPoints(pack, "i") }],
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
      [{ name: "P", points: global.WattcycleHistory.toPoints(pack, "p") }],
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
      [{ name: "Ah", color: "#c084fc", points: global.WattcycleHistory.toPoints(pack, "ah") }],
      { unit: "Ah", yDigits: 1, guides: capacityGuides }
    );
    global.WattcycleCharts.drawSeries(document.getElementById("chartTemps"), [
      { name: "MOS", color: "#fb7185", points: global.WattcycleHistory.toPoints(pack, "mos") },
      { name: "PCB", color: "#38bdf8", points: global.WattcycleHistory.toPoints(pack, "pcb") }
    ], { legend: "MOS / PCB", unit: "°C", yDigits: 1 });

    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarVoltage"),
      [{ name: "V", color: "#60a5fa", points: global.WattcycleSolarHistory.toPoints(solar, "v") }],
      { unit: "V", yDigits: 2 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarCurrent"),
      [{ name: "I", color: "#3ecf8e", points: global.WattcycleSolarHistory.toPoints(solar, "i") }],
      { unit: "A", yDigits: 3 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarPower"),
      [{ name: "P", color: "#f0b429", points: global.WattcycleSolarHistory.toPoints(solar, "p") }],
      { unit: "W", yDigits: 1 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarEnergy"),
      [{ name: "Wh", color: "#c084fc", points: global.WattcycleSolarHistory.toPoints(solar, "wh") }],
      { unit: "Wh", yDigits: 3 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistSolarCapacity"),
      [{ name: "Ah", color: "#38bdf8", points: global.WattcycleSolarHistory.toPoints(solar, "ah") }],
      { unit: "Ah", yDigits: 3 }
    );
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.history = renderHistory;
})(window);
