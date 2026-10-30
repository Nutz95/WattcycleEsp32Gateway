(function (global) {
  function parseHm(text) {
    const parts = String(text || "0:0").split(":");
    const h = Number(parts[0]) || 0;
    const m = Number(parts[1]) || 0;
    return h * 60 + m;
  }

  function inInterval(minute, start, end) {
    if (start === end) return true;
    if (start < end) return minute >= start && minute < end;
    return minute >= start || minute < end;
  }

  function windowMatchesDate(windowCfg, dateYmd) {
    const from = String(windowCfg.from || "01-01");
    const to = String(windowCfg.to || "12-31");
    const md = dateYmd.slice(5);
    if (from.length === 10 && to.length === 10) {
      return dateYmd >= from && dateYmd <= to;
    }
    if (from <= to) return md >= from && md <= to;
    return md >= from || md <= to;
  }

  function bandAt(config, epochMs) {
    const date = new Date(epochMs);
    const ymd =
      date.getFullYear() +
      "-" +
      String(date.getMonth() + 1).padStart(2, "0") +
      "-" +
      String(date.getDate()).padStart(2, "0");
    const minute = date.getHours() * 60 + date.getMinutes();
    const dow = date.getDay();
    const windows = (config && config.windows) || [];
    for (let w = 0; w < windows.length; w += 1) {
      const win = windows[w];
      if (!windowMatchesDate(win, ymd)) continue;
      const bands = win.bands || [];
      for (let b = 0; b < bands.length; b += 1) {
        const band = bands[b];
        if (Array.isArray(band.weekdays) && band.weekdays.length > 0) {
          if (band.weekdays.indexOf(dow) < 0) continue;
        }
        const intervals = band.intervals || [];
        for (let i = 0; i < intervals.length; i += 1) {
          const start = parseHm(intervals[i].start);
          const end = parseHm(intervals[i].end);
          if (inInterval(minute, start, end)) {
            return band;
          }
        }
      }
    }
    return null;
  }

  function emptyBuckets() {
    return {
      byBand: {},
      byEquipment: {},
      totalWh: 0,
      totalCost: 0,
    };
  }

  function ensureEquip(buckets, equipment) {
    if (!buckets.byEquipment[equipment]) {
      buckets.byEquipment[equipment] = { wh: 0, cost: 0, byBand: {} };
    }
    if (!buckets.byEquipment[equipment].byBand) {
      buckets.byEquipment[equipment].byBand = {};
    }
    return buckets.byEquipment[equipment];
  }

  function accumulateSegment(buckets, bandName, equipment, wh, rate) {
    if (!(wh > 0)) return;
    if (!buckets.byBand[bandName]) {
      buckets.byBand[bandName] = { wh: 0, cost: 0 };
    }
    const equip = ensureEquip(buckets, equipment);
    if (!equip.byBand[bandName]) {
      equip.byBand[bandName] = { wh: 0, cost: 0 };
    }
    const cost = (wh / 1000) * (Number.isFinite(rate) ? rate : 0);
    buckets.byBand[bandName].wh += wh;
    buckets.byBand[bandName].cost += cost;
    equip.wh += wh;
    equip.cost += cost;
    equip.byBand[bandName].wh += wh;
    equip.byBand[bandName].cost += cost;
    buckets.totalWh += wh;
    buckets.totalCost += cost;
  }

  function integratePowerByBand(config, points, powerKey) {
    const byBand = {};
    let total = 0;
    for (let i = 1; i < points.length; i += 1) {
      const prev = points[i - 1];
      const cur = points[i];
      const dtH = (cur.t - prev.t) / 3600000;
      if (dtH <= 0 || dtH > 1) continue;
      const p = Number(prev[powerKey]);
      if (!Number.isFinite(p) || p <= 0) continue;
      const wh = p * dtH;
      const band = bandAt(config, prev.t);
      const name = band ? band.name : "unpriced";
      if (!byBand[name]) byBand[name] = { wh: 0, rate: band ? Number(band.euroPerKwh) : 0 };
      byBand[name].wh += wh;
      if (band) byBand[name].rate = Number(band.euroPerKwh);
      total += wh;
    }
    return { byBand: byBand, total: total };
  }

  function findNamedRates(config) {
    let hp = NaN;
    let hc = NaN;
    const windows = (config && config.windows) || [];
    windows.forEach(function (win) {
      (win.bands || []).forEach(function (band) {
        const name = String(band.name || "").toUpperCase();
        if (name === "HP" && !Number.isFinite(hp)) hp = Number(band.euroPerKwh);
        if (name === "HC" && !Number.isFinite(hc)) hc = Number(band.euroPerKwh);
      });
    });
    return { hp: hp, hc: hc };
  }

  /// Scale sample Wh-per-band so totals match meterWh (Shelly counter), then price.
  function allocateMeterWh(config, points, powerKey, meterWh) {
    const integrated = integratePowerByBand(config, points, powerKey);
    const target = Number.isFinite(meterWh) && meterWh > 0 ? meterWh : integrated.total;
    const allocated = {};
    if (!(target > 0)) return allocated;
    if (integrated.total > 0) {
      const scale = target / integrated.total;
      Object.keys(integrated.byBand).forEach(function (name) {
        allocated[name] = {
          wh: integrated.byBand[name].wh * scale,
          rate: integrated.byBand[name].rate,
        };
      });
      return allocated;
    }
    // No usable power samples: bill whole meter Wh on the band at mid-span (or unpriced).
    const mid =
      points.length > 0 ? points[Math.floor(points.length / 2)].t : Date.now();
    const band = bandAt(config, mid);
    const name = band ? band.name : "unpriced";
    allocated[name] = {
      wh: target,
      rate: band ? Number(band.euroPerKwh) : 0,
    };
    return allocated;
  }

  function applyAllocation(buckets, equipment, allocated) {
    Object.keys(allocated).forEach(function (name) {
      const row = allocated[name];
      const rate = Number.isFinite(row.rate) ? row.rate : 0;
      accumulateSegment(buckets, name, equipment, row.wh, rate);
    });
  }

  function costPowerSeries(config, points, powerKey, equipment, buckets) {
    const integrated = integratePowerByBand(config, points, powerKey);
    Object.keys(integrated.byBand).forEach(function (name) {
      accumulateSegment(
        buckets,
        name,
        equipment,
        integrated.byBand[name].wh,
        integrated.byBand[name].rate
      );
    });
  }

  function costPackDay(config, dayAgg) {
    const buckets = emptyBuckets();
    if (!dayAgg) return buckets;
    costPowerSeries(config, dayAgg.packPoints || [], "p", "pack_charge", buckets);
    costPowerSeries(config, dayAgg.ecoPoints || [], "acIn", "eco_ac_in", buckets);
    return buckets;
  }

  function costEcoDay(config, dayAgg) {
    const buckets = emptyBuckets();
    if (!dayAgg) return buckets;
    costPowerSeries(config, dayAgg.ecoPoints || [], "acIn", "eco_ac_in", buckets);
    return buckets;
  }

  function costPlugDay(config, plugDay) {
    const buckets = emptyBuckets();
    if (!plugDay || !plugDay.plugs) return buckets;
    plugDay.plugs.forEach(function (plug) {
      const key = "plug:" + plug.id;
      const meterWh =
        typeof plug.wh === "number"
          ? plug.wh
          : global.WattcycleReportsPlugEnergy.plugDayWh(plug);
      const allocated = allocateMeterWh(config, plug.points || [], "p", meterWh);
      applyAllocation(buckets, key, allocated);
    });
    return buckets;
  }

  function mergeBuckets(into, from) {
    Object.keys(from.byBand).forEach(function (key) {
      if (!into.byBand[key]) into.byBand[key] = { wh: 0, cost: 0 };
      into.byBand[key].wh += from.byBand[key].wh;
      into.byBand[key].cost += from.byBand[key].cost;
    });
    Object.keys(from.byEquipment).forEach(function (key) {
      const src = from.byEquipment[key];
      const dst = ensureEquip(into, key);
      dst.wh += src.wh || 0;
      dst.cost += src.cost || 0;
      Object.keys(src.byBand || {}).forEach(function (band) {
        if (!dst.byBand[band]) dst.byBand[band] = { wh: 0, cost: 0 };
        dst.byBand[band].wh += src.byBand[band].wh;
        dst.byBand[band].cost += src.byBand[band].cost;
      });
    });
    into.totalWh += from.totalWh;
    into.totalCost += from.totalCost;
    return into;
  }

  /// Savings = energy billed in HC × (HP − HC). Positive = money saved vs all-HP.
  function hcVersusHpSavings(config, buckets) {
    const rates = findNamedRates(config);
    if (!Number.isFinite(rates.hp) || !Number.isFinite(rates.hc)) {
      return { savings: NaN, hcWh: 0, hpWh: 0, costIfAllHp: NaN, actual: buckets.totalCost };
    }
    let hcWh = 0;
    let hpWh = 0;
    Object.keys(buckets.byBand || {}).forEach(function (name) {
      const upper = String(name).toUpperCase();
      const wh = buckets.byBand[name].wh || 0;
      if (upper === "HC") hcWh += wh;
      if (upper === "HP") hpWh += wh;
    });
    const savings = (hcWh / 1000) * (rates.hp - rates.hc);
    const costIfAllHp = (buckets.totalWh / 1000) * rates.hp;
    return {
      savings: savings,
      hcWh: hcWh,
      hpWh: hpWh,
      costIfAllHp: costIfAllHp,
      actual: buckets.totalCost,
      hpRate: rates.hp,
      hcRate: rates.hc,
    };
  }

  global.WattcycleReportsCosting = {
    bandAt: bandAt,
    costPackDay: costPackDay,
    costEcoDay: costEcoDay,
    costPlugDay: costPlugDay,
    mergeBuckets: mergeBuckets,
    emptyBuckets: emptyBuckets,
    hcVersusHpSavings: hcVersusHpSavings,
    findNamedRates: findNamedRates,
  };
})(window);
