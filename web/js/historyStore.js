/* Browser-side history: append every poll, grow window up to 24h. */
(function (global) {
  const KEY = "wattcycle.history.v3";
  const MAX_AGE_MS = 24 * 60 * 60 * 1000;
  const MAX_POINTS = 2880;
  const PERSIST_EVERY_MS = 15000;

  let memoryPoints = null;
  let lastPersistMs = 0;

  function loadPersisted() {
    try {
      const raw = localStorage.getItem(KEY);
      if (!raw) return [];
      const parsed = JSON.parse(raw);
      return Array.isArray(parsed) ? parsed : [];
    } catch (error) {
      return [];
    }
  }

  function savePersisted(points) {
    try {
      localStorage.setItem(KEY, JSON.stringify(points));
    } catch (error) {
      const trimmed = downsample(points, Math.floor(MAX_POINTS / 2));
      try {
        localStorage.setItem(KEY, JSON.stringify(trimmed));
      } catch (ignored) {}
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
    // Always append a new point per poll so the curve forms; replace only if
    // two samples land in the same millisecond (unlikely).
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
      localStorage.removeItem(KEY);
    } catch (ignored) {}
    return memoryPoints;
  }

  global.WattcycleHistory = { pushSample: pushSample, all: all, clear: clear };
})(window);
