(function (global) {
  let bound = false;
  let loading = false;
  let tariffConfig = null;
  let lastFlow = null;
  let lastPlugs = null;
  let lastEcoCost = null;
  let activePanel = "flow";

  function activatePanel(name) {
    activePanel = name;
    document.querySelectorAll("[data-reports-tab]").forEach(function (tab) {
      tab.classList.toggle("active", tab.getAttribute("data-reports-tab") === name);
    });
    document.querySelectorAll("[data-reports-panel]").forEach(function (panel) {
      panel.hidden = panel.getAttribute("data-reports-panel") !== name;
    });
    // Hidden canvases get width 0 — redraw after the panel is visible.
    global.requestAnimationFrame(function () {
      global.requestAnimationFrame(function () {
        redrawActiveCharts();
      });
    });
  }

  function fmtWh(wh) {
    if (!Number.isFinite(wh)) return "—";
    if (Math.abs(wh) >= 1000) return (wh / 1000).toFixed(2) + " kWh";
    return wh.toFixed(0) + " Wh";
  }

  function fmtCost(cost, currency) {
    if (!Number.isFinite(cost)) return "—";
    return cost.toFixed(2) + " " + (currency || "EUR");
  }

  function setText(id, text) {
    const el = document.getElementById(id);
    if (el) el.textContent = text;
  }

  function bandWh(equip, bandName) {
    if (!equip || !equip.byBand) return 0;
    const upper = String(bandName).toUpperCase();
    let total = 0;
    Object.keys(equip.byBand).forEach(function (name) {
      if (String(name).toUpperCase() === upper) total += equip.byBand[name].wh || 0;
    });
    return total;
  }

  function bandCost(equip, bandName) {
    if (!equip || !equip.byBand) return 0;
    const upper = String(bandName).toUpperCase();
    let total = 0;
    Object.keys(equip.byBand).forEach(function (name) {
      if (String(name).toUpperCase() === upper) total += equip.byBand[name].cost || 0;
    });
    return total;
  }

  function redrawActiveCharts() {
    if (activePanel === "flow" && lastFlow) {
      renderFlowCharts(lastFlow.roll, lastFlow.cost);
    }
    if (activePanel === "plugs" && lastPlugs) {
      renderPlugCharts(lastPlugs.roll, lastPlugs.cost);
    }
  }

  function bucketsToHpHc(buckets) {
    const hp = bandWh({ byBand: buckets.byBand }, "HP");
    const hc = bandWh({ byBand: buckets.byBand }, "HC");
    const other = Math.max(0, (buckets.totalWh || 0) - hp - hc);
    return { HP: hp, HC: hc, other: other };
  }

  function renderFlowCharts(roll, cost) {
    const currency = (tariffConfig && tariffConfig.currency) || "EUR";
    const daily = (roll.days || []).filter(Boolean);
    // Stack Eco AC-in Wh by tariff band (same HP/HC colours as plugs).
    const cats = daily.map(function (day) {
      const ecoBuckets = global.WattcycleReportsCosting.costEcoDay(tariffConfig, day);
      return {
        label: global.WattcycleDateFormat.ymdToDmy(day.date).slice(0, 5),
        values: bucketsToHpHc(ecoBuckets),
      };
    });
    global.WattcycleReportsBars.drawCategoryBars(
      document.getElementById("chartRepFlowBars"),
      cats,
      {
        bands: ["HP", "HC", "other"],
        bandColors: { HP: "#f0b429", HC: "#60a5fa", other: "#94a3b8" },
        legend: true,
        emptyLabel: "No Eco AC-in energy",
      }
    );
    // Pie: Eco cost share by band (not pack+solar total).
    const ecoRoll = global.WattcycleReportsCosting.emptyBuckets();
    daily.forEach(function (day) {
      global.WattcycleReportsCosting.mergeBuckets(
        ecoRoll,
        global.WattcycleReportsCosting.costEcoDay(tariffConfig, day)
      );
    });
    const pieSource = ecoRoll.totalWh > 0 ? ecoRoll : cost;
    const slices = Object.keys(pieSource.byBand).map(function (name) {
      return { label: name, value: pieSource.byBand[name].cost };
    });
    global.WattcycleReportsPie.drawPie(document.getElementById("chartRepFlowPie"), slices, {
      unit: " " + currency,
      digits: 2,
      emptyLabel: "No priced energy",
    });
  }

  function renderPlugCharts(roll, cost) {
    const currency = (tariffConfig && tariffConfig.currency) || "EUR";
    const cats = (roll.plugs || []).map(function (plug) {
      const equip = cost.byEquipment["plug:" + plug.id] || { byBand: {} };
      return {
        label: plug.name || plug.id,
        values: {
          HP: bandWh(equip, "HP"),
          HC: bandWh(equip, "HC"),
          other: Math.max(
            0,
            (equip.wh || plug.wh || 0) - bandWh(equip, "HP") - bandWh(equip, "HC")
          ),
        },
      };
    });
    global.WattcycleReportsBars.drawCategoryBars(
      document.getElementById("chartRepPlugBars"),
      cats,
      {
        bands: ["HP", "HC", "other"],
        bandColors: { HP: "#f0b429", HC: "#60a5fa", other: "#94a3b8" },
        legend: true,
        emptyLabel: "No plug energy",
      }
    );
    const slices = (roll.plugs || []).map(function (plug) {
      return { label: plug.name || plug.id, value: plug.wh };
    });
    global.WattcycleReportsPie.drawPie(document.getElementById("chartRepPlugPie"), slices, {
      unit: " Wh",
      digits: 0,
      emptyLabel: "No plug energy",
    });
  }

  function renderFlow(roll, cost, ecoCost) {
    lastFlow = { roll: roll, cost: cost };
    lastEcoCost = ecoCost || global.WattcycleReportsCosting.emptyBuckets();
    const currency = (tariffConfig && tariffConfig.currency) || "EUR";
    setText("repPackIn", fmtWh(roll.packInWh));
    setText("repPackOut", fmtWh(roll.packOutWh));
    setText("repSolar", fmtWh(roll.solarWh));
    setText(
      "repSoc",
      Number.isFinite(roll.socMin) && Number.isFinite(roll.socMax)
        ? roll.socMin.toFixed(0) + "% → " + roll.socMax.toFixed(0) + "%"
        : "—"
    );
    setText("repEcoIn", fmtWh(roll.ecoAcInWh));
    setText("repEcoCost", fmtCost(lastEcoCost.totalCost, currency));
    const savings = global.WattcycleReportsCosting.hcVersusHpSavings(tariffConfig, lastEcoCost);
    setText(
      "repEcoSavings",
      Number.isFinite(savings.savings)
        ? fmtCost(savings.savings, currency) +
            " (HC " +
            fmtWh(savings.hcWh) +
            ")"
        : "—"
    );
    setText("repFlowCost", fmtCost(cost.totalCost, currency));
    if (activePanel === "flow") renderFlowCharts(roll, cost);
  }

  function renderPlugs(roll, cost) {
    lastPlugs = { roll: roll, cost: cost };
    const currency = (tariffConfig && tariffConfig.currency) || "EUR";
    setText("repPlugsTotal", fmtWh(roll.totalWh));
    setText("repPlugsCost", fmtCost(cost.totalCost, currency));
    const savings = global.WattcycleReportsCosting.hcVersusHpSavings(tariffConfig, cost);
    setText(
      "repPlugsSavings",
      Number.isFinite(savings.savings) ? fmtCost(savings.savings, currency) : "—"
    );
    if (activePanel === "plugs") renderPlugCharts(roll, cost);
    const tbody = document.querySelector("#reportsPlugTable tbody");
    if (!tbody) return;
    tbody.innerHTML = "";
    (roll.plugs || []).forEach(function (plug) {
      const equip = cost.byEquipment["plug:" + plug.id] || { wh: 0, cost: 0, byBand: {} };
      const wh = Number.isFinite(equip.wh) && equip.wh > 0 ? equip.wh : plug.wh;
      const hpW = bandWh(equip, "HP");
      const hcW = bandWh(equip, "HC");
      const hpC = bandCost(equip, "HP");
      const hcC = bandCost(equip, "HC");
      const avg = wh > 0 ? equip.cost / (wh / 1000) : NaN;
      const tr = document.createElement("tr");
      tr.innerHTML =
        "<td>" +
        (plug.name || plug.id) +
        "</td><td>" +
        fmtWh(wh) +
        "</td><td>" +
        fmtWh(hpW) +
        "</td><td>" +
        fmtWh(hcW) +
        "</td><td>" +
        fmtCost(hpC, currency) +
        "</td><td>" +
        fmtCost(hcC, currency) +
        "</td><td>" +
        fmtCost(equip.cost, currency) +
        "</td><td>" +
        (Number.isFinite(avg) ? avg.toFixed(3) : "—") +
        "</td>";
      tbody.appendChild(tr);
    });
  }

  function setLoading(active, done, total, message) {
    loading = !!active;
    const load = document.getElementById("reportsLoad");
    const progress = document.getElementById("reportsProgress");
    const status = document.getElementById("reportsStatus");
    const refreshBtn = document.getElementById("reportsRefreshBtn");
    if (load) {
      if (active) load.removeAttribute("hidden");
      else load.setAttribute("hidden", "");
    }
    if (refreshBtn) refreshBtn.disabled = !!active;
    if (progress) {
      progress.max = Math.max(1, total || 1);
      progress.value = Math.min(progress.max, done || 0);
    }
    if (status && message) status.textContent = message;
  }

  async function loadTariffsIntoUi() {
    tariffConfig = await global.WattcycleTariffStore.load();
    const editorRoot = document.getElementById("reportsTariffEditor");
    if (editorRoot && global.WattcycleTariffEditor) {
      global.WattcycleTariffEditor.mount(editorRoot, tariffConfig, function (draft) {
        tariffConfig = draft;
      });
    }
  }

  async function fetchPackDay(date) {
    try {
      const csv = await global.WattcycleHistoryDayCache.fetchDayCsv(
        "pack",
        date,
        "/api/history/day"
      );
      return global.WattcycleReportsDayAggregate.aggregatePackDay(csv, date);
    } catch (error) {
      return null;
    }
  }

  async function fetchPlugDay(date) {
    try {
      const csv = await global.WattcycleHistoryDayCache.fetchDayCsv(
        "plugs",
        date,
        "/api/history/plugs/day"
      );
      return global.WattcycleReportsPlugEnergy.aggregatePlugsDay(csv, date);
    } catch (error) {
      return null;
    }
  }

  async function refresh() {
    if (loading) return;
    const periodEl = document.getElementById("reportsPeriod");
    const anchorEl = document.getElementById("reportsAnchor");
    const status = document.getElementById("reportsStatus");
    if (!periodEl || !anchorEl || !anchorEl.value) {
      if (status) status.textContent = "Choose an anchor day.";
      setLoading(false, 0, 1, status ? status.textContent : "");
      return;
    }
    if (!tariffConfig) {
      await loadTariffsIntoUi();
    }
    const bounds = global.WattcycleReportsPeriod.periodBounds(anchorEl.value, periodEl.value);
    const fromLabel = global.WattcycleDateFormat.ymdToDmy(bounds.from);
    const toLabel = global.WattcycleDateFormat.ymdToDmy(bounds.to);
    const totalSteps = Math.max(1, bounds.days.length * 2);
    let doneSteps = 0;
    setLoading(
      true,
      0,
      totalSteps,
      "Loading " + bounds.days.length + " day(s) " + fromLabel + " → " + toLabel + "…"
    );
    function tick(kind) {
      doneSteps += 1;
      setLoading(
        true,
        doneSteps,
        totalSteps,
        "Loading " + kind + "… " + doneSteps + "/" + totalSteps +
          " (" + fromLabel + " → " + toLabel + ")"
      );
    }
    try {
      const packDays = await global.WattcycleReportsPeriod.mapPool(
        bounds.days,
        3,
        async function (date) {
          const day = await fetchPackDay(date);
          tick("pack");
          return day;
        }
      );
      const plugDays = await global.WattcycleReportsPeriod.mapPool(
        bounds.days,
        3,
        async function (date) {
          const day = await fetchPlugDay(date);
          tick("plugs");
          return day;
        }
      );
      const packRoll = global.WattcycleReportsPeriod.sumPackDays(packDays);
      const plugRoll = global.WattcycleReportsPeriod.sumPlugDays(plugDays);
      const flowCost = global.WattcycleReportsCosting.emptyBuckets();
      const ecoCost = global.WattcycleReportsCosting.emptyBuckets();
      packDays.forEach(function (day) {
        if (!day) return;
        global.WattcycleReportsCosting.mergeBuckets(
          flowCost,
          global.WattcycleReportsCosting.costPackDay(tariffConfig, day)
        );
        global.WattcycleReportsCosting.mergeBuckets(
          ecoCost,
          global.WattcycleReportsCosting.costEcoDay(tariffConfig, day)
        );
      });
      const plugCost = global.WattcycleReportsCosting.emptyBuckets();
      plugDays.forEach(function (day) {
        if (!day) return;
        global.WattcycleReportsCosting.mergeBuckets(
          plugCost,
          global.WattcycleReportsCosting.costPlugDay(tariffConfig, day)
        );
      });
      // Prefer priced Wh on plug rollup totals for consistency with the table.
      let pricedWh = 0;
      (plugRoll.plugs || []).forEach(function (plug) {
        const equip = plugCost.byEquipment["plug:" + plug.id];
        if (equip && equip.wh > 0) {
          plug.wh = equip.wh;
        }
        pricedWh += plug.wh || 0;
      });
      plugRoll.totalWh = pricedWh;
      renderFlow(packRoll, flowCost, ecoCost);
      renderPlugs(plugRoll, plugCost);
      const loaded = packDays.filter(Boolean).length;
      setLoading(
        false,
        totalSteps,
        totalSteps,
        "Loaded " + loaded + "/" + bounds.days.length + " pack days; plugs " +
          plugDays.filter(Boolean).length + "/" + bounds.days.length + "."
      );
    } catch (error) {
      setLoading(false, 0, 1, "Load failed: " + (error && error.message ? error.message : "error"));
    }
  }

  function bind() {
    if (bound) return;
    bound = true;
    setLoading(false, 0, 1, "Preparing…");
    const anchor = document.getElementById("reportsAnchor");
    if (anchor && !anchor.value) {
      anchor.value = global.WattcycleHistoryDayCache.utcToday();
    }
    document.querySelectorAll("[data-reports-tab]").forEach(function (tab) {
      tab.addEventListener("click", function () {
        activatePanel(tab.getAttribute("data-reports-tab"));
      });
    });
    const refreshBtn = document.getElementById("reportsRefreshBtn");
    if (refreshBtn) refreshBtn.addEventListener("click", refresh);
    const saveBtn = document.getElementById("reportsTariffSaveBtn");
    if (saveBtn) {
      saveBtn.addEventListener("click", async function () {
        const status = document.getElementById("reportsTariffStatus");
        try {
          const draft = global.WattcycleTariffEditor
            ? global.WattcycleTariffEditor.getDraft()
            : tariffConfig;
          tariffConfig = await global.WattcycleTariffStore.saveValidated(draft);
          if (global.WattcycleTariffEditor) {
            global.WattcycleTariffEditor.setDraft(tariffConfig);
          }
          if (status) status.textContent = "Tarifs enregistrés dans ce navigateur.";
          if (lastFlow) renderFlow(lastFlow.roll, lastFlow.cost, lastEcoCost);
          if (lastPlugs) renderPlugs(lastPlugs.roll, lastPlugs.cost);
        } catch (error) {
          if (status) status.textContent = "Enregistrement refusé : " + error.message;
        }
      });
    }
    const resetBtn = document.getElementById("reportsTariffResetBtn");
    if (resetBtn) {
      resetBtn.addEventListener("click", function () {
        const defaults = global.WattcycleTariffStore.defaultConfig();
        if (global.WattcycleTariffEditor) {
          global.WattcycleTariffEditor.setDraft(defaults);
        }
        tariffConfig = defaults;
        const status = document.getElementById("reportsTariffStatus");
        if (status) status.textContent = "Défaut rechargé — cliquez Enregistrer pour persister.";
      });
    }
    const exportBtn = document.getElementById("reportsTariffExportBtn");
    if (exportBtn) {
      exportBtn.addEventListener("click", function () {
        const draft = global.WattcycleTariffEditor
          ? global.WattcycleTariffEditor.getDraft()
          : tariffConfig || global.WattcycleTariffStore.defaultConfig();
        try {
          const clean = global.WattcycleTariffValidate.validateConfig(draft);
          const blob = new Blob([global.WattcycleTariffStore.exportJson(clean)], {
            type: "application/json",
          });
          const url = URL.createObjectURL(blob);
          const a = document.createElement("a");
          a.href = url;
          a.download = "wattcycle-tariffs.json";
          a.click();
          URL.revokeObjectURL(url);
        } catch (error) {
          const status = document.getElementById("reportsTariffStatus");
          if (status) status.textContent = "Export refusé : " + error.message;
        }
      });
    }
    const importFile = document.getElementById("reportsTariffImportFile");
    if (importFile) {
      importFile.addEventListener("change", function () {
        const file = importFile.files && importFile.files[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = function () {
          const status = document.getElementById("reportsTariffStatus");
          try {
            const parsed = global.WattcycleTariffStore.importJson(String(reader.result || ""));
            if (global.WattcycleTariffEditor) {
              global.WattcycleTariffEditor.setDraft(parsed);
            }
            tariffConfig = parsed;
            if (status) status.textContent = "Import OK — cliquez Enregistrer pour persister.";
          } catch (error) {
            if (status) status.textContent = "Import refusé : " + error.message;
          }
        };
        reader.readAsText(file);
      });
    }
    loadTariffsIntoUi().then(function () {
      refresh();
    });
  }

  global.WattcycleReports = {
    bind: bind,
    refresh: refresh,
  };
})(window);
