/* Browser-side history stores: pack (BMS) + solar (XT369P), up to 24h. */
(function (global) {
  const MAX_AGE_MS = 24 * 60 * 60 * 1000;
  const MAX_POINTS = 2880;
  const PERSIST_EVERY_MS = 15000;

  function createStore(storageKey) {
    let memoryPoints = null;
    let lastPersistMs = 0;

    function loadPersisted() {
      try {
        const raw = localStorage.getItem(storageKey);
        if (!raw) return [];
        const parsed = JSON.parse(raw);
        return Array.isArray(parsed) ? parsed : [];
      } catch (error) {
        return [];
      }
    }

    function downsample(points, targetCount) {
      if (points.length <= targetCount) {
        return points.slice();
      }
      const out = [];
      const lastIndex = points.length - 1;
      for (let i = 0; i < targetCount; i += 1) {
        const index = Math.round((i * lastIndex) / (targetCount - 1));
        out.push(points[index]);
      }
      return out;
    }

    function savePersisted(points) {
      try {
        localStorage.setItem(storageKey, JSON.stringify(points));
      } catch (error) {
        const trimmed = downsample(points, Math.floor(MAX_POINTS / 2));
        try {
          localStorage.setItem(storageKey, JSON.stringify(trimmed));
        } catch (ignored) {}
      }
    }

    function prune(points, now) {
      let next = points.filter(function (point) {
        return now - point.t <= MAX_AGE_MS;
      });
      if (next.length > MAX_POINTS) {
        next = downsample(next, MAX_POINTS);
      }
      return next;
    }

    function ensureLoaded() {
      if (memoryPoints == null) {
        memoryPoints = loadPersisted();
      }
      return memoryPoints;
    }

    function pushSample(sample) {
      const now = Date.now();
      let points = prune(ensureLoaded(), now);
      const last = points[points.length - 1];
      if (last && sample.t === last.t) {
        points[points.length - 1] = sample;
      } else {
        points.push(sample);
      }
      points = prune(points, now);
      memoryPoints = points;
      if (lastPersistMs === 0 || now - lastPersistMs >= PERSIST_EVERY_MS) {
        savePersisted(points);
        lastPersistMs = now;
      }
      return points;
    }

    function all() {
      return prune(ensureLoaded(), Date.now());
    }

    function clear() {
      memoryPoints = [];
      lastPersistMs = 0;
      try {
        localStorage.removeItem(storageKey);
      } catch (ignored) {}
      return memoryPoints;
    }

    function toPoints(history, key) {
      return (history || [])
        .filter(function (sample) {
          return typeof sample.t === "number" && typeof sample[key] === "number";
        })
        .map(function (sample) {
          return { t: sample.t, v: sample[key] };
        });
    }

    return {
      pushSample: pushSample,
      all: all,
      clear: clear,
      toPoints: toPoints
    };
  }

  global.WattcycleHistory = createStore("wattcycle.history.v3");
  global.WattcycleSolarHistory = createStore("wattcycle.solar.history.v1");
})(window);
