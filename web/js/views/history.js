(function (global) {
  let lastTotalAh = 0;
  let bound = false;
  let todayMerged = false;
  let activeDay = "";
  let dayOverlayPack = null;
  let dayOverlaySolar = null;
  let dayOverlayEco = null;
  let loadingDay = false;
  let catalogGeneration = 0;
  let monthFollowsYear = false;
  const MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];

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
    return global.WattcycleHistoryCsv.parsePackDay(csv);
  }

  function fillSelect(select, values, selected, textOf) {
    select.innerHTML = "";
    values.forEach(function (value) {
      const opt = document.createElement("option");
      opt.value = String(value);
      opt.textContent = textOf(value);
      select.appendChild(opt);
    });
    select.value = String(selected);
  }

  function catalogOf(body) {
    const now = new Date();
    const fallbackYear = now.getUTCFullYear();
    const fallbackMonth = now.getUTCMonth() + 1;
    if (Array.isArray(body.years)) {
      const years = body.years.length ? body.years.slice() : [fallbackYear];
      const months = body.months && body.months.length ? body.months.slice() : [fallbackMonth];
      return {
        year: body.year || fallbackYear,
        month: body.month || fallbackMonth,
        years: years,
        months: months,
        days: body.days || []
      };
    }
    const years = [];
    const months = [];
    (body.days || []).forEach(function (day) {
      const year = Number(String(day).slice(0, 4));
      const month = Number(String(day).slice(5, 7));
      if (years.indexOf(year) < 0) years.push(year);
      if (months.indexOf(month) < 0) months.push(month);
    });
    return {
      year: fallbackYear,
      month: fallbackMonth,
      years: years.length ? years : [fallbackYear],
      months: months.length ? months : [fallbackMonth],
      days: body.days || []
    };
  }

  async function refreshSdDays() {
    const statusEl = document.getElementById("historySdStatus");
    const yearEl = document.getElementById("historyYearSelect");
    const monthEl = document.getElementById("historyMonthSelect");
    const select = document.getElementById("historyDaySelect");
    if (!statusEl || !select) return;
    const generation = catalogGeneration + 1;
    catalogGeneration = generation;
    const params = [];
    if (yearEl && yearEl.value) params.push("year=" + encodeURIComponent(yearEl.value));
    if (!monthFollowsYear && monthEl && monthEl.value) {
      params.push("month=" + encodeURIComponent(monthEl.value));
    }
    monthFollowsYear = false;
    const query = params.length ? "?" + params.join("&") : "";
    try {
      const res = await fetch("/api/history/days" + query, { credentials: "same-origin" });
      if (generation !== catalogGeneration) return;
      if (!res.ok) {
        statusEl.textContent = "SD history unavailable (login / hub offline).";
        return;
      }
      const body = await res.json();
      if (generation !== catalogGeneration) return;
      global.WattcycleDevice = Object.assign({}, global.WattcycleDevice || {}, body);
      const catalog = catalogOf(body);
      if (yearEl) fillSelect(yearEl, catalog.years, catalog.year, function (year) { return String(year); });
      if (monthEl) {
        fillSelect(monthEl, catalog.months, catalog.month, function (month) {
          return MONTHS[month - 1] || String(month);
        });
      }
      statusEl.textContent =
        "NTP " + (body.ntp ? "synced" : "pending") + " · SD " + (body.sd ? "ready" : "missing");
      const current = select.value;
      select.innerHTML = '<option value="">Live + today (localStorage)</option>';
      catalog.days.forEach(function (day) {
        const opt = document.createElement("option");
        opt.value = day;
        opt.textContent = global.WattcycleDateFormat.ymdToDmy(day);
        select.appendChild(opt);
      });
      if (current && catalog.days.indexOf(current) >= 0) select.value = current;

      const today = utcDayString(Date.now());
      if (!todayMerged && body.sd && catalog.days.indexOf(today) >= 0) {
        todayMerged = true;
        await loadDayIntoCharts(today, { mergeIntoLive: true, selectDay: false });
      }
    } catch (err) {
      if (generation === catalogGeneration) statusEl.textContent = "SD history unreachable.";
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
    const dayLabel = global.WattcycleDateFormat.ymdToDmy(date);
    if (statusEl) statusEl.textContent = "Downloading " + dayLabel + "…";
    try {
      const text = await global.WattcycleHistoryDayCache.fetchDayCsv(
        "pack",
        date,
        "/api/history/day",
        {
          onProgress: function (received, total) {
            if (progress && total > 0) {
              const pct = Math.min(100, Math.round((received / total) * 100));
              progress.value = pct;
              if (statusEl) {
                statusEl.textContent =
                  "Downloading " + dayLabel + "… " + pct + "% (" + Math.round(received / 1024) + " KB)";
              }
            } else if (statusEl) {
              statusEl.textContent =
                "Downloading " + dayLabel + "… " + Math.round(received / 1024) + " KB";
            }
          },
        }
      );
      if (progress) {
        progress.value = 100;
        progress.hidden = true;
      }
      if (statusEl) {
        statusEl.textContent =
          "Loaded " + dayLabel + " (" + Math.round(text.length / 1024) + " KB csv)";
      }
      return text;
    } catch (error) {
      if (progress) progress.hidden = true;
      if (statusEl) {
        statusEl.textContent =
          "Day download failed" + (error && error.status ? " (" + error.status + ")" : "") + ".";
      }
      return null;
    }
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
      const dayLabel = global.WattcycleDateFormat.ymdToDmy(date);
      if (opts.mergeIntoLive) {
        global.WattcycleHistory.mergeSamples(parsed.pack);
        global.WattcycleSolarHistory.mergeSamples(parsed.solar);
        global.WattcycleEcoflowHistory.mergeSamples(parsed.eco);
        activeDay = "";
        dayOverlayPack = null;
        dayOverlaySolar = null;
        dayOverlayEco = null;
        if (global.WattcycleHistoryPlugs) {
          await global.WattcycleHistoryPlugs.loadDay(date);
        }
        if (statusEl) {
          statusEl.textContent =
            "Merged " + dayLabel + " from SD (" + parsed.pack.length + " pack / " +
            parsed.solar.length + " solar pts) into live charts";
        }
      } else {
        activeDay = date;
        dayOverlayPack = parsed.pack;
        dayOverlaySolar = parsed.solar;
        dayOverlayEco = parsed.eco;
        if (global.WattcycleHistoryPlugs) {
          await global.WattcycleHistoryPlugs.loadDay(date);
        }
        if (opts.selectDay !== false) {
          const select = document.getElementById("historyDaySelect");
          if (select) select.value = date;
        }
        if (statusEl) {
          statusEl.textContent =
            "Showing SD day " + dayLabel + " (" + parsed.pack.length + " pack / " +
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
        const name = tab.getAttribute("data-history-tab");
        activateHistoryTab(name);
        if (name === "plugs" && global.WattcycleHistoryPlugs) {
          global.WattcycleHistoryPlugs.render();
        }
      });
    });
    if (global.WattcycleHistoryPlugs) {
      global.WattcycleHistoryPlugs.bindFilter();
    }
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
    const clearEcoflow = document.getElementById("clearEcoflowHistoryBtn");
    if (clearEcoflow) {
      clearEcoflow.addEventListener("click", function () {
        global.WattcycleEcoflowHistory.clear();
      });
    }
    const yearEl = document.getElementById("historyYearSelect");
    const monthEl = document.getElementById("historyMonthSelect");
    if (yearEl) {
      yearEl.addEventListener("change", function () {
        monthFollowsYear = true;
        refreshSdDays();
      });
    }
    if (monthEl) {
      monthEl.addEventListener("change", function () {
        refreshSdDays();
      });
    }
    const select = document.getElementById("historyDaySelect");
    if (select) {
      select.addEventListener("change", function () {
        if (!select.value) {
          activeDay = "";
          dayOverlayPack = null;
          dayOverlaySolar = null;
          dayOverlayEco = null;
          if (global.WattcycleHistoryPlugs) {
            global.WattcycleHistoryPlugs.loadDay(utcDayString(Date.now()));
          }
          const statusEl = document.getElementById("historySdStatus");
          if (statusEl) statusEl.textContent = "Live charts (localStorage + today SD merge)";
          return;
        }
        loadDayIntoCharts(select.value, { mergeIntoLive: false, selectDay: true });
      });
    }
    refreshSdDays();
  }

  function renderHistory(data, packHistory, solarHistory, ecoflowHistory) {
    bind();
    if (data.valid && typeof data.totalAh === "number" && data.totalAh > 0) {
      lastTotalAh = data.totalAh;
    }

    const pack = activeDay && dayOverlayPack !== null ? dayOverlayPack : packHistory;
    const solar = activeDay && dayOverlaySolar !== null ? dayOverlaySolar : solarHistory;

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

    const ecoHist = activeDay && dayOverlayEco !== null
      ? dayOverlayEco
      : ecoflowHistory || global.WattcycleEcoflowHistory.all();
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistEcoSoc"),
      [{ name: "SoC", color: "#3ecf8e", points: global.WattcycleEcoflowHistory.toPoints(ecoHist, "soc") }],
      { unit: "%", yDigits: 0 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistEcoAcOut"),
      [{ name: "AC out", color: "#f0b429", points: global.WattcycleEcoflowHistory.toPoints(ecoHist, "acOut") }],
      { unit: "W", yDigits: 0 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistEcoAcIn"),
      [{ name: "AC in", color: "#60a5fa", points: global.WattcycleEcoflowHistory.toPoints(ecoHist, "acIn") }],
      { unit: "W", yDigits: 0 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistEcoDcOut"),
      [{ name: "DC", color: "#c084fc", points: global.WattcycleEcoflowHistory.toPoints(ecoHist, "dcOut") }],
      { unit: "W", yDigits: 0 }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistEcoUsbOut"),
      [{ name: "USB", color: "#38bdf8", points: global.WattcycleEcoflowHistory.toPoints(ecoHist, "usbOut") }],
      { unit: "W", yDigits: 0 }
    );
    if (global.WattcycleHistoryPlugs) {
      global.WattcycleHistoryPlugs.render();
    }
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.history = renderHistory;
})(window);
