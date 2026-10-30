(function (global) {
  function parseYmd(date) {
    const parts = String(date || "").split("-");
    if (parts.length !== 3) return null;
    const y = Number(parts[0]);
    const m = Number(parts[1]);
    const d = Number(parts[2]);
    if (!Number.isFinite(y) || !Number.isFinite(m) || !Number.isFinite(d)) return null;
    return { y: y, m: m, d: d };
  }

  function ymdString(y, m, d) {
    return (
      String(y) +
      "-" +
      String(m).padStart(2, "0") +
      "-" +
      String(d).padStart(2, "0")
    );
  }

  function addDays(ymd, delta) {
    const utc = Date.UTC(ymd.y, ymd.m - 1, ymd.d + delta);
    const dt = new Date(utc);
    return {
      y: dt.getUTCFullYear(),
      m: dt.getUTCMonth() + 1,
      d: dt.getUTCDate(),
    };
  }

  function listDaysInRange(fromDate, toDate) {
    const from = parseYmd(fromDate);
    const to = parseYmd(toDate);
    if (!from || !to) return [];
    const days = [];
    let cursor = from;
    for (let guard = 0; guard < 400; guard += 1) {
      days.push(ymdString(cursor.y, cursor.m, cursor.d));
      if (cursor.y === to.y && cursor.m === to.m && cursor.d === to.d) break;
      cursor = addDays(cursor, 1);
    }
    return days;
  }

  function mondayOf(date) {
    const ymd = parseYmd(date);
    if (!ymd) return date;
    const dt = new Date(Date.UTC(ymd.y, ymd.m - 1, ymd.d));
    const dow = dt.getUTCDay();
    const back = dow === 0 ? 6 : dow - 1;
    const monday = addDays(ymd, -back);
    return ymdString(monday.y, monday.m, monday.d);
  }

  function periodBounds(anchorDate, period) {
    const ymd = parseYmd(anchorDate);
    if (!ymd) {
      return { from: anchorDate, to: anchorDate, days: [anchorDate] };
    }
    if (period === "day") {
      const day = ymdString(ymd.y, ymd.m, ymd.d);
      return { from: day, to: day, days: [day] };
    }
    if (period === "week") {
      const from = mondayOf(anchorDate);
      const fromYmd = parseYmd(from);
      const toYmd = addDays(fromYmd, 6);
      const to = ymdString(toYmd.y, toYmd.m, toYmd.d);
      return { from: from, to: to, days: listDaysInRange(from, to) };
    }
    if (period === "month") {
      const from = ymdString(ymd.y, ymd.m, 1);
      const last = new Date(Date.UTC(ymd.y, ymd.m, 0)).getUTCDate();
      const to = ymdString(ymd.y, ymd.m, last);
      return { from: from, to: to, days: listDaysInRange(from, to) };
    }
    const from = ymdString(ymd.y, 1, 1);
    const to = ymdString(ymd.y, 12, 31);
    return { from: from, to: to, days: listDaysInRange(from, to) };
  }

  async function mapPool(items, concurrency, worker) {
    const results = new Array(items.length);
    let next = 0;
    async function runOne() {
      for (;;) {
        const index = next;
        next += 1;
        if (index >= items.length) return;
        results[index] = await worker(items[index], index);
      }
    }
    const runners = [];
    const count = Math.max(1, Math.min(concurrency || 3, items.length || 1));
    for (let i = 0; i < count; i += 1) {
      runners.push(runOne());
    }
    await Promise.all(runners);
    return results;
  }

  function sumPackDays(days) {
    const roll = {
      packInWh: 0,
      packOutWh: 0,
      solarWh: 0,
      ecoAcInWh: 0,
      ecoAcOutWh: 0,
      socMin: NaN,
      socMax: NaN,
      days: [],
    };
    days.forEach(function (day) {
      if (!day) return;
      roll.packInWh += day.packInWh || 0;
      roll.packOutWh += day.packOutWh || 0;
      roll.solarWh += day.solarWh || 0;
      roll.ecoAcInWh += day.ecoAcInWh || 0;
      roll.ecoAcOutWh += day.ecoAcOutWh || 0;
      if (Number.isFinite(day.socMin)) {
        if (!Number.isFinite(roll.socMin) || day.socMin < roll.socMin) roll.socMin = day.socMin;
      }
      if (Number.isFinite(day.socMax)) {
        if (!Number.isFinite(roll.socMax) || day.socMax > roll.socMax) roll.socMax = day.socMax;
      }
      roll.days.push(day);
    });
    return roll;
  }

  function sumPlugDays(days) {
    const byId = {};
    days.forEach(function (day) {
      if (!day || !day.plugs) return;
      day.plugs.forEach(function (plug) {
        if (!byId[plug.id]) {
          byId[plug.id] = { id: plug.id, name: plug.name, wh: 0 };
        }
        byId[plug.id].name = plug.name || byId[plug.id].name;
        byId[plug.id].wh += plug.wh || 0;
      });
    });
    const plugs = Object.keys(byId)
      .sort()
      .map(function (id) {
        return byId[id];
      });
    let totalWh = 0;
    plugs.forEach(function (plug) {
      totalWh += plug.wh;
    });
    return { plugs: plugs, totalWh: totalWh, days: days.filter(Boolean) };
  }

  global.WattcycleReportsPeriod = {
    periodBounds: periodBounds,
    listDaysInRange: listDaysInRange,
    mapPool: mapPool,
    sumPackDays: sumPackDays,
    sumPlugDays: sumPlugDays,
  };
})(window);
