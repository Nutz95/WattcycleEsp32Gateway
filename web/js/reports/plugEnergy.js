(function (global) {
  function parsePlugsCsv(csv) {
    return global.WattcycleHistoryCsv.parsePlugsDay(csv);
  }

  function plugDayWh(plug) {
    const points = plug.points || [];
    if (points.length === 0) return 0;
    const first = points[0];
    const last = points[points.length - 1];
    // Prefer lifetime counter delta (robust across mid-day starts).
    if (Number.isFinite(last.whTotal) && Number.isFinite(first.whTotal) && last.whTotal >= first.whTotal) {
      const deltaTotal = last.whTotal - first.whTotal;
      if (deltaTotal > 0 || points.length === 1) return Math.max(0, deltaTotal);
    }
    if (Number.isFinite(last.whToday) && Number.isFinite(first.whToday) && last.whToday >= first.whToday) {
      return Math.max(0, last.whToday - first.whToday);
    }
    let wh = 0;
    for (let i = 1; i < points.length; i += 1) {
      const prev = points[i - 1];
      const cur = points[i];
      const dtH = (cur.t - prev.t) / 3600000;
      if (dtH <= 0 || dtH > 1) continue;
      const p = Number(prev.p);
      if (!Number.isFinite(p) || p <= 0) continue;
      wh += p * dtH;
    }
    return wh;
  }

  function aggregatePlugsDay(csv, date) {
    const byId = parsePlugsCsv(csv);
    const plugs = Object.keys(byId)
      .sort()
      .map(function (id) {
        const plug = byId[id];
        return {
          id: id,
          name: plug.name || id,
          wh: plugDayWh(plug),
          points: plug.points,
        };
      });
    let totalWh = 0;
    plugs.forEach(function (plug) {
      totalWh += plug.wh;
    });
    return { date: date, plugs: plugs, totalWh: totalWh };
  }

  global.WattcycleReportsPlugEnergy = {
    parsePlugsCsv: parsePlugsCsv,
    plugDayWh: plugDayWh,
    aggregatePlugsDay: aggregatePlugsDay,
  };
})(window);
