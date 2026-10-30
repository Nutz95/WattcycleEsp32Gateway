(function (global) {
  const TIME_RE = /^([01]\d|2[0-3]):([0-5]\d)$/;
  const MD_RE = /^(0[1-9]|1[0-2])-(0[1-9]|[12]\d|3[01])$/;
  const CURRENCY_RE = /^[A-Za-z]{3}$/;

  function fail(message) {
    const err = new Error(message);
    err.code = "tariff_invalid";
    return err;
  }

  function mdToOrdinal(md) {
    const parts = String(md).split("-");
    return Number(parts[0]) * 100 + Number(parts[1]);
  }

  function windowsOverlap(a, b) {
    const aFrom = mdToOrdinal(a.from);
    const aTo = mdToOrdinal(a.to);
    const bFrom = mdToOrdinal(b.from);
    const bTo = mdToOrdinal(b.to);
    if (aFrom <= aTo && bFrom <= bTo) {
      return aFrom <= bTo && bFrom <= aTo;
    }
    // Year-wrap windows: treat as overlap if either wraps or ranges touch.
    return true;
  }

  function validateConfig(config) {
    if (!config || typeof config !== "object") {
      throw fail("Configuration manquante.");
    }
    const currency = String(config.currency || "").trim().toUpperCase();
    if (!CURRENCY_RE.test(currency)) {
      throw fail("Devise invalide (code ISO à 3 lettres, ex. EUR).");
    }
    if (!Array.isArray(config.windows) || config.windows.length === 0) {
      throw fail("Ajoutez au moins une période de validité.");
    }
    if (config.windows.length > 12) {
      throw fail("Maximum 12 périodes de validité.");
    }
    const windows = [];
    for (let w = 0; w < config.windows.length; w += 1) {
      const win = config.windows[w] || {};
      const from = String(win.from || "").trim();
      const to = String(win.to || "").trim();
      if (!MD_RE.test(from) || !MD_RE.test(to)) {
        throw fail("Période " + (w + 1) + " : dates au format MM-JJ (ex. 01-01).");
      }
      if (!Array.isArray(win.bands) || win.bands.length === 0) {
        throw fail("Période " + (w + 1) + " : au moins une bande (HP/HC…).");
      }
      if (win.bands.length > 8) {
        throw fail("Période " + (w + 1) + " : maximum 8 bandes.");
      }
      const bands = [];
      for (let b = 0; b < win.bands.length; b += 1) {
        const band = win.bands[b] || {};
        const name = String(band.name || "").trim();
        if (!name || name.length > 16) {
          throw fail("Bande " + (b + 1) + " : nom requis (max 16 caractères).");
        }
        const rate = Number(band.euroPerKwh);
        if (!Number.isFinite(rate) || rate < 0 || rate > 5) {
          throw fail("Bande « " + name + " » : prix entre 0 et 5 €/kWh.");
        }
        if (!Array.isArray(band.intervals) || band.intervals.length === 0) {
          throw fail("Bande « " + name + " » : au moins un créneau horaire.");
        }
        if (band.intervals.length > 8) {
          throw fail("Bande « " + name + " » : maximum 8 créneaux.");
        }
        const intervals = [];
        for (let i = 0; i < band.intervals.length; i += 1) {
          const start = String((band.intervals[i] || {}).start || "").trim();
          const end = String((band.intervals[i] || {}).end || "").trim();
          if (!TIME_RE.test(start) || !TIME_RE.test(end)) {
            throw fail("Bande « " + name + " » : horaires HH:MM (00–23).");
          }
          if (start === end) {
            throw fail("Bande « " + name + " » : début et fin identiques interdits.");
          }
          intervals.push({ start: start, end: end });
        }
        bands.push({
          name: name,
          euroPerKwh: Math.round(rate * 10000) / 10000,
          intervals: intervals,
        });
      }
      const normalized = {
        id: String(win.id || "w" + (w + 1)),
        from: from,
        to: to,
        bands: bands,
      };
      for (let prev = 0; prev < windows.length; prev += 1) {
        if (windowsOverlap(windows[prev], normalized)) {
          throw fail("Les périodes " + (prev + 1) + " et " + (w + 1) + " se chevauchent.");
        }
      }
      windows.push(normalized);
    }
    return { currency: currency, windows: windows };
  }

  global.WattcycleTariffValidate = {
    validateConfig: validateConfig,
    TIME_RE: TIME_RE,
    MD_RE: MD_RE,
  };
})(window);
