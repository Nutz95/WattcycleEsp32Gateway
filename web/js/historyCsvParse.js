(function (global) {
  function numOrUndef(raw) {
    if (raw === undefined || raw === null || raw === "") return undefined;
    const n = Number(raw);
    return Number.isFinite(n) ? n : undefined;
  }

  function parsePackDay(csv) {
    // v1: epoch,soc,pack_v,pack_a,pack_w,solar_v,solar_a,solar_w,pack_ok,solar_ok
    // v2: + pack_ah,mos_c,pcb_c,solar_wh,solar_ah + eco columns
    const pack = [];
    const solar = [];
    const eco = [];
    if (!csv) return { pack: pack, solar: solar, eco: eco };
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
      const hasExt = cols.length >= 15;
      if (packOk) {
        const row = {
          t: t,
          v: Number(cols[2]),
          i: Number(cols[3]),
          p: Number(cols[4]),
          soc: Number(cols[1]),
        };
        if (hasExt) {
          const ah = numOrUndef(cols[10]);
          const mos = numOrUndef(cols[11]);
          const pcb = numOrUndef(cols[12]);
          if (ah !== undefined) row.ah = ah;
          if (mos !== undefined) row.mos = mos;
          if (pcb !== undefined) row.pcb = pcb;
        }
        pack.push(row);
      }
      if (solarOk) {
        const row = {
          t: t,
          v: Number(cols[5]),
          i: Number(cols[6]),
          p: Number(cols[7]),
        };
        if (hasExt) {
          const wh = numOrUndef(cols[13]);
          const ah = numOrUndef(cols[14]);
          if (wh !== undefined) row.wh = wh;
          if (ah !== undefined) row.ah = ah;
        }
        solar.push(row);
      }
      if (cols.length >= 21 && cols[15] === "1") {
        eco.push({
          t: t,
          soc: Number(cols[16]),
          acOut: Number(cols[17]),
          acIn: Number(cols[18]),
          dcOut: Number(cols[19]),
          usbOut: Number(cols[20]),
        });
      }
    }
    return { pack: pack, solar: solar, eco: eco };
  }

  function parsePlugsDay(csv) {
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
        whTotal: Number(cols[4]),
        whToday: Number(cols[5]),
      });
    }
    return byId;
  }

  global.WattcycleHistoryCsv = {
    parsePackDay: parsePackDay,
    parsePlugsDay: parsePlugsDay,
  };
})(window);
