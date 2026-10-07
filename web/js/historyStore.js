/* Browser-side 24h history (localStorage) with live tip updates each refresh. */
(function (global) {
  const KEY = "wattcycle.history.v2";
  const MAX_AGE_MS = 24 * 60 * 60 * 1000;
  const MAX_POINTS = 2880; // 30s * 2880 ~= 24h
  const APPEND_MS = 30000;

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
      const trimmed = points.slice(Math.floor(points.length / 2));
      try {
        localStorage.setItem(KEY, JSON.stringify(trimmed));
      } catch (ignored) {}
    }
  }

  function prune(points, now) {
    let next = points.filter(function (point) {
      return now - point.t <= MAX_AGE_MS;
    });
    if (next.length > MAX_POINTS) {
      next = next.slice(next.length - MAX_POINTS);
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
    if (!last || now - last.t >= APPEND_MS) {
      points.push(sample);
    } else {
      points[points.length - 1] = sample;
    }
    points = prune(points, now);
    memoryPoints = points;
    if (lastPersistMs === 0 || now - lastPersistMs >= APPEND_MS) {
      savePersisted(points);
      lastPersistMs = now;
    }
    return points;
  }

  function all() {
    return prune(ensureLoaded(), Date.now());
  }

  global.WattcycleHistory = { pushSample: pushSample, all: all };
})(window);
