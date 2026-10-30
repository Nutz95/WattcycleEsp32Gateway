(function (global) {
  let dayOverlayPlugs = null;
  let selectedPlugId = "";
  let pendingPlugFocus = "";
  const PLUG_COLORS = ["#f0b429", "#3ecf8e", "#60a5fa", "#c084fc", "#fb7185", "#38bdf8", "#a3e635", "#f97316"];

  function parsePlugsCsv(csv) {
    return global.WattcycleHistoryCsv.parsePlugsDay(csv);
  }

  function plugSeriesList(byId, key) {
    const ids = Object.keys(byId).sort();
    const filtered = selectedPlugId ? ids.filter(function (id) { return id === selectedPlugId; }) : ids;
    return filtered.map(function (id, index) {
      const plug = byId[id];
      return {
        name: plug.name || id,
        color: PLUG_COLORS[index % PLUG_COLORS.length],
        points: plug.points.map(function (row) {
          return { t: row.t, v: row[key] };
        }),
      };
    });
  }

  function refreshPlugFilter(byId) {
    const select = document.getElementById("historyPlugSelect");
    if (!select) return;
    const previous = selectedPlugId || pendingPlugFocus || select.value;
    select.innerHTML = "";
    const all = document.createElement("option");
    all.value = "";
    all.textContent = "All plugs";
    select.appendChild(all);
    Object.keys(byId).sort().forEach(function (id) {
      const opt = document.createElement("option");
      opt.value = id;
      opt.textContent = (byId[id].name || id) + " (" + id.slice(-6) + ")";
      select.appendChild(opt);
    });
    if (previous && byId[previous]) {
      select.value = previous;
      selectedPlugId = previous;
    } else {
      select.value = "";
      selectedPlugId = "";
    }
    pendingPlugFocus = "";
  }

  function render() {
    const byId = dayOverlayPlugs || {};
    if (!global.WattcycleCharts) return;
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistPlugPower"),
      plugSeriesList(byId, "p"),
      { unit: "W", yDigits: 0, legend: "Plug power" }
    );
    global.WattcycleCharts.drawSeries(
      document.getElementById("chartHistPlugWhToday"),
      plugSeriesList(byId, "whToday"),
      { unit: "Wh", yDigits: 1, legend: "Wh today" }
    );
  }

  async function loadDay(date) {
    if (!date) {
      dayOverlayPlugs = null;
      refreshPlugFilter({});
      render();
      return;
    }
    const statusEl = document.getElementById("historySdStatus");
    const dayLabel = global.WattcycleDateFormat
      ? global.WattcycleDateFormat.ymdToDmy(date)
      : date;
    try {
      const csv = await global.WattcycleHistoryDayCache.fetchDayCsv(
        "plugs",
        date,
        "/api/history/plugs/day"
      );
      dayOverlayPlugs = parsePlugsCsv(csv);
      refreshPlugFilter(dayOverlayPlugs);
      render();
      const count = Object.keys(dayOverlayPlugs).length;
      if (statusEl) {
        statusEl.textContent =
          "Plugs " + dayLabel + " : " + count + " device(s), " +
          Math.round((csv || "").length / 1024) + " KB";
      }
    } catch (error) {
      dayOverlayPlugs = {};
      refreshPlugFilter({});
      render();
      if (statusEl) {
        statusEl.textContent =
          "Plugs day failed" + (error && error.status ? " (" + error.status + ")" : "") +
          " for " + dayLabel;
      }
    }
  }

  function clear() {
    dayOverlayPlugs = null;
    refreshPlugFilter({});
    render();
  }

  function bindFilter() {
    const plugSelect = document.getElementById("historyPlugSelect");
    if (!plugSelect || plugSelect.getAttribute("data-bound")) return;
    plugSelect.setAttribute("data-bound", "1");
    plugSelect.addEventListener("change", function () {
      selectedPlugId = plugSelect.value || "";
      render();
    });
  }

  function focusPlug(plugId) {
    pendingPlugFocus = plugId || "";
    selectedPlugId = plugId || "";
    document.querySelectorAll(".subtab").forEach(function (tab) {
      tab.classList.toggle("active", tab.getAttribute("data-history-tab") === "plugs");
    });
    document.querySelectorAll(".history-panel").forEach(function (panel) {
      panel.hidden = panel.getAttribute("data-history-panel") !== "plugs";
    });
    const daySelect = document.getElementById("historyDaySelect");
    const date = (daySelect && daySelect.value) || utcToday();
    loadDay(date);
    if (daySelect && !daySelect.value) {
      daySelect.value = date;
    }
  }

  function utcToday() {
    const d = new Date();
    const y = d.getUTCFullYear();
    const m = String(d.getUTCMonth() + 1).padStart(2, "0");
    const day = String(d.getUTCDate()).padStart(2, "0");
    return y + "-" + m + "-" + day;
  }

  global.WattcycleHistoryPlugs = {
    bindFilter: bindFilter,
    loadDay: loadDay,
    clear: clear,
    render: render,
    focusPlug: focusPlug,
  };
})(window);
