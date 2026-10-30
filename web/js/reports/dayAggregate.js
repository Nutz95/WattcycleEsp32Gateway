(function (global) {
  function integratePositiveWh(points, powerKey) {
    let wh = 0;
    for (let i = 1; i < points.length; i += 1) {
      const prev = points[i - 1];
      const cur = points[i];
      const dtH = (cur.t - prev.t) / 3600000;
      if (dtH <= 0 || dtH > 1) continue;
      const p = Number(prev[powerKey]);
      if (!Number.isFinite(p) || p <= 0) continue;
      wh += p * dtH;
    }
    return wh;
  }

  function integrateSignedWh(points, powerKey) {
    let inWh = 0;
    let outWh = 0;
    for (let i = 1; i < points.length; i += 1) {
      const prev = points[i - 1];
      const cur = points[i];
      const dtH = (cur.t - prev.t) / 3600000;
      if (dtH <= 0 || dtH > 1) continue;
      const p = Number(prev[powerKey]);
      if (!Number.isFinite(p)) continue;
      if (p > 0) inWh += p * dtH;
      else if (p < 0) outWh += -p * dtH;
    }
    return { inWh: inWh, outWh: outWh };
  }

  function parsePackDayCsv(csv) {
    return global.WattcycleHistoryCsv.parsePackDay(csv);
  }

  function solarWhFromSeries(solar) {
    if (solar.length === 0) return 0;
    const withWh = solar.filter(function (row) {
      return Number.isFinite(row.wh);
    });
    if (withWh.length >= 2) {
      return Math.max(0, withWh[withWh.length - 1].wh - withWh[0].wh);
    }
    return integratePositiveWh(solar, "p");
  }

  function aggregatePackDay(csv, date) {
    const parsed = parsePackDayCsv(csv);
    const packEnergy = integrateSignedWh(parsed.pack, "p");
    let socMin = NaN;
    let socMax = NaN;
    parsed.pack.forEach(function (row) {
      if (!Number.isFinite(row.soc)) return;
      if (!Number.isFinite(socMin) || row.soc < socMin) socMin = row.soc;
      if (!Number.isFinite(socMax) || row.soc > socMax) socMax = row.soc;
    });
    const ecoIn = integratePositiveWh(parsed.eco, "acIn");
    const ecoOut = integratePositiveWh(parsed.eco, "acOut");
    return {
      date: date,
      packInWh: packEnergy.inWh,
      packOutWh: packEnergy.outWh,
      solarWh: solarWhFromSeries(parsed.solar),
      socMin: socMin,
      socMax: socMax,
      ecoAcInWh: ecoIn,
      ecoAcOutWh: ecoOut,
      packPoints: parsed.pack,
      solarPoints: parsed.solar,
      ecoPoints: parsed.eco,
    };
  }

  global.WattcycleReportsDayAggregate = {
    parsePackDayCsv: parsePackDayCsv,
    aggregatePackDay: aggregatePackDay,
    integratePositiveWh: integratePositiveWh,
    integrateSignedWh: integrateSignedWh,
  };
})(window);
