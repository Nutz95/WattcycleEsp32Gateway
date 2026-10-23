(function (global) {
  let dayOverlayPlugs = null;
  let selectedPlugId = "";
  let pendingPlugFocus = "";
  const PLUG_COLORS = ["#f0b429", "#3ecf8e", "#60a5fa", "#c084fc", "#fb7185", "#38bdf8", "#a3e635", "#f97316"];

  function parsePlugsCsv(csv) {
    // epoch,id,name,watts,wh_total,wh_today,output
    const byId = {};
    if (!csv) return byId;
    const lines = csv.split(/\r?\n/);
    for (let i = 0; i < lines.length; i += 1) {
      const line = lines[i].trim();
      if (!line || line.indexOf("epoch") === 0) continue;
      const cols = line.split(",");
      if (cols.length < 7) continue;
      const epoch = Number(cols[0]);
      if (!Number.isFinite(epoch) || epoch <= 0) continue;
      const id = cols[1];
      if (!id) continue;
      if (!byId[id]) {
        byId[id] = { id: id, name: cols[2] || id, points: [] };
      }
      byId[id].name = cols[2] || byId[id].name;
      byId[id].points.push({
        t: epoch * 1000,
        p: Number(cols[3]),
        whToday: Number(cols[5]),
      });
    }
    return byId;
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
    try {
      const response = await fetch("/api/history/plugs/day?date=" + encodeURIComponent(date), {
        credentials: "same-origin",
        cache: "no-store",
      });
      if (!response.ok) {
        dayOverlayPlugs = {};
        refreshPlugFilter({});
        render();
        return;
      }
      dayOverlayPlugs = parsePlugsCsv(await response.text());
      refreshPlugFilter(dayOverlayPlugs);
      render();
    } catch (error) {
      dayOverlayPlugs = {};
      refreshPlugFilter({});
      render();
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
