/* Time-adaptive canvas charts: Y labels, guides, signed colors, pointer crosshair. */
(function (global) {
  const MAX_WINDOW_MS = 24 * 60 * 60 * 1000;
  const MIN_SPAN_MS = 1000;
  const MS_PER_SECOND = 1000;
  const MS_PER_MINUTE = 60 * MS_PER_SECOND;
  const MS_PER_HOUR = 60 * MS_PER_MINUTE;
  const DEFAULT_CANVAS_WIDTH = 320;
  const DEFAULT_CANVAS_HEIGHT = 120;
  const GRID_LINES = 3;
  const VALUE_PAD_RATIO = 0.08;
  const FLAT_RANGE_PAD = 1;
  const PLOT_LEFT = 44;
  const PLOT_RIGHT = 8;
  const PLOT_TOP = 18;
  const PLOT_BOTTOM = 4;
  const CHART_BG = "rgba(255,255,255,0.03)";
  const GRID_STROKE = "rgba(255,255,255,0.08)";
  const ZERO_STROKE = "rgba(255,255,255,0.28)";
  const GUIDE_STROKE = "rgba(240,180,41,0.55)";
  const CROSSHAIR_STROKE = "rgba(232,238,252,0.55)";
  const LABEL_FILL = "#93a4c7";
  const DEFAULT_SERIES_COLOR = "#3ecf8e";
  const POSITIVE_COLOR = "#3ecf8e";
  const NEGATIVE_COLOR = "#ff6b6b";
  const LABEL_FONT = "11px Segoe UI, sans-serif";
  const LINE_WIDTH = 1.6;
  const SINGLE_POINT_RADIUS = 2.5;
  const CROSSHAIR_RADIUS = 3;
  /// Break the polyline when samples are farther apart than this (missing data).
  const GAP_BREAK_MS = 60 * 1000;
  const chartState = new WeakMap();

  function syncCanvasSize(canvas) {
    const cssWidth = Math.max(1, Math.floor(canvas.clientWidth || canvas.width || DEFAULT_CANVAS_WIDTH));
    const cssHeight = Math.max(1, Math.floor(canvas.clientHeight || DEFAULT_CANVAS_HEIGHT));
    if (canvas.width !== cssWidth) canvas.width = cssWidth;
    if (canvas.height !== cssHeight) canvas.height = cssHeight;
    return { width: cssWidth, height: cssHeight };
  }

  function collectPoints(seriesList) {
    const points = [];
    seriesList.forEach(function (series) {
      (series.points || []).forEach(function (point) {
        if (typeof point.t === "number" && typeof point.v === "number" &&
            !Number.isNaN(point.t) && !Number.isNaN(point.v)) {
          points.push(point);
        }
      });
    });
    return points;
  }

  function formatWindowLabel(spanMs) {
    if (spanMs < MS_PER_MINUTE) {
      return Math.max(1, Math.round(spanMs / MS_PER_SECOND)) + "s window";
    }
    if (spanMs < MS_PER_HOUR) {
      return Math.max(1, Math.round(spanMs / MS_PER_MINUTE)) + "m window";
    }
    return (spanMs / MS_PER_HOUR).toFixed(1) + "h window";
  }

  function formatAxisValue(value, digits) {
    const places = typeof digits === "number" ? digits : 1;
    return Math.abs(value) >= 100 ? value.toFixed(0) : value.toFixed(places);
  }

  function formatTime(ms) {
    const date = new Date(ms);
    return [date.getHours(), date.getMinutes(), date.getSeconds()]
      .map(function (n) { return String(n).padStart(2, "0"); })
      .join(":");
  }

  function plotBox(size) {
    return {
      left: PLOT_LEFT,
      top: PLOT_TOP,
      width: Math.max(1, size.width - PLOT_LEFT - PLOT_RIGHT),
      height: Math.max(1, size.height - PLOT_TOP - PLOT_BOTTOM)
    };
  }

  function mapX(t, tMin, span, box) {
    return box.left + ((t - tMin) / span) * (box.width - 1);
  }

  function mapY(v, vMin, vMax, box) {
    return box.top + box.height - ((v - vMin) / (vMax - vMin)) * (box.height - 1);
  }

  function signOf(value) {
    return value > 0 ? 1 : (value < 0 ? -1 : 0);
  }

  function seriesColor(series, opts, value) {
    if (opts.signed || series.signed) {
      return value < 0
        ? (series.negativeColor || opts.negativeColor || NEGATIVE_COLOR)
        : (series.positiveColor || opts.positiveColor || POSITIVE_COLOR);
    }
    return series.color || DEFAULT_SERIES_COLOR;
  }

  function drawPolyline(graphics, points, tMin, span, vMin, vMax, box, color) {
    if (!points.length) return;
    graphics.strokeStyle = color;
    graphics.fillStyle = color;
    graphics.lineWidth = LINE_WIDTH;
    if (points.length === 1) {
      graphics.beginPath();
      graphics.arc(mapX(points[0].t, tMin, span, box), mapY(points[0].v, vMin, vMax, box),
        SINGLE_POINT_RADIUS, 0, Math.PI * 2);
      graphics.fill();
      return;
    }
    let pathOpen = false;
    for (let index = 0; index < points.length; index += 1) {
      const point = points[index];
      const x = mapX(point.t, tMin, span, box);
      const y = mapY(point.v, vMin, vMax, box);
      const gap = index > 0 && (point.t - points[index - 1].t) > GAP_BREAK_MS;
      if (index === 0 || gap) {
        if (pathOpen) {
          graphics.stroke();
        }
        graphics.beginPath();
        graphics.moveTo(x, y);
        pathOpen = true;
      } else {
        graphics.lineTo(x, y);
      }
    }
    if (pathOpen) {
      graphics.stroke();
    }
  }

  function drawSignedPolyline(graphics, points, tMin, span, vMin, vMax, box, posColor, negColor) {
    if (!points.length) return;
    let segment = [points[0]];
    let activeSign = signOf(points[0].v) || 1;
    function flush() {
      drawPolyline(graphics, segment, tMin, span, vMin, vMax, box,
        activeSign < 0 ? negColor : posColor);
    }
    for (let i = 1; i < points.length; i += 1) {
      const point = points[i];
      const prev = points[i - 1];
      if ((point.t - prev.t) > GAP_BREAK_MS) {
        flush();
        segment = [point];
        activeSign = signOf(point.v) || 1;
        continue;
      }
      const nextSign = signOf(point.v) || activeSign;
      if (nextSign !== activeSign) {
        const denom = Math.abs(prev.v) + Math.abs(point.v);
        const frac = denom > 0 ? Math.abs(prev.v) / denom : 0.5;
        const zeroPoint = { t: prev.t + (point.t - prev.t) * frac, v: 0 };
        segment.push(zeroPoint);
        flush();
        segment = [zeroPoint, point];
        activeSign = nextSign;
      } else {
        segment.push(point);
      }
    }
    flush();
  }

  function nearestPoint(points, tTarget) {
    let best = null;
    let bestDist = Infinity;
    points.forEach(function (point) {
      const dist = Math.abs(point.t - tTarget);
      if (dist < bestDist) {
        bestDist = dist;
        best = point;
      }
    });
    return best;
  }

  function visiblePoints(series, tMin) {
    return (series.points || []).filter(function (point) {
      return point.t >= tMin && typeof point.v === "number" && !Number.isNaN(point.v);
    });
  }

  function paintChart(canvas, seriesList, options, pointerX) {
    const size = syncCanvasSize(canvas);
    const graphics = canvas.getContext("2d");
    if (!graphics) return;
    const box = plotBox(size);
    const opts = options || {};

    graphics.clearRect(0, 0, size.width, size.height);
    graphics.fillStyle = CHART_BG;
    graphics.fillRect(0, 0, size.width, size.height);

    const allPoints = collectPoints(seriesList);
    if (!allPoints.length) {
      graphics.fillStyle = LABEL_FILL;
      graphics.font = LABEL_FONT;
      graphics.fillText("Waiting for samples…", PLOT_LEFT, PLOT_TOP);
      return;
    }

    let tMin = Infinity;
    let tMax = -Infinity;
    let vMin = Infinity;
    let vMax = -Infinity;
    allPoints.forEach(function (point) {
      if (point.t < tMin) tMin = point.t;
      if (point.t > tMax) tMax = point.t;
      if (point.v < vMin) vMin = point.v;
      if (point.v > vMax) vMax = point.v;
    });
    (opts.guides || []).forEach(function (guide) {
      if (typeof guide.v === "number" && !Number.isNaN(guide.v)) {
        if (guide.v < vMin) vMin = guide.v;
        if (guide.v > vMax) vMax = guide.v;
      }
    });
    if (opts.signed) {
      if (vMin > 0) vMin = 0;
      if (vMax < 0) vMax = 0;
    }

    let span = Math.max(MIN_SPAN_MS, tMax - tMin);
    if (span > MAX_WINDOW_MS) {
      tMin = tMax - MAX_WINDOW_MS;
      span = MAX_WINDOW_MS;
    }
    if (vMax === vMin) {
      vMax += FLAT_RANGE_PAD;
      vMin -= FLAT_RANGE_PAD;
    }
    const pad = (vMax - vMin) * VALUE_PAD_RATIO;
    vMin -= pad;
    vMax += pad;

    graphics.strokeStyle = GRID_STROKE;
    graphics.beginPath();
    for (let i = 0; i <= GRID_LINES + 1; i += 1) {
      const y = box.top + (box.height * i) / (GRID_LINES + 1);
      graphics.moveTo(box.left, y);
      graphics.lineTo(box.left + box.width, y);
    }
    graphics.stroke();

    graphics.fillStyle = LABEL_FILL;
    graphics.font = LABEL_FONT;
    graphics.textAlign = "right";
    graphics.textBaseline = "middle";
    for (let i = 0; i <= GRID_LINES + 1; i += 1) {
      const ratio = i / (GRID_LINES + 1);
      graphics.fillText(formatAxisValue(vMax - (vMax - vMin) * ratio, opts.yDigits),
        PLOT_LEFT - 4, box.top + box.height * ratio);
    }
    graphics.textAlign = "left";
    graphics.textBaseline = "alphabetic";

    if (vMin < 0 && vMax > 0) {
      const zeroY = mapY(0, vMin, vMax, box);
      graphics.strokeStyle = ZERO_STROKE;
      graphics.beginPath();
      graphics.moveTo(box.left, zeroY);
      graphics.lineTo(box.left + box.width, zeroY);
      graphics.stroke();
    }

    (opts.guides || []).forEach(function (guide) {
      if (typeof guide.v !== "number" || Number.isNaN(guide.v)) return;
      const y = mapY(guide.v, vMin, vMax, box);
      graphics.strokeStyle = guide.color || GUIDE_STROKE;
      graphics.setLineDash([4, 4]);
      graphics.beginPath();
      graphics.moveTo(box.left, y);
      graphics.lineTo(box.left + box.width, y);
      graphics.stroke();
      graphics.setLineDash([]);
      if (guide.label) {
        graphics.fillStyle = guide.color || GUIDE_STROKE;
        graphics.fillText(guide.label, box.left + 4, y - 3);
      }
    });

    seriesList.forEach(function (series) {
      const points = visiblePoints(series, tMin);
      if (!points.length) return;
      if (opts.signed || series.signed) {
        drawSignedPolyline(graphics, points, tMin, span, vMin, vMax, box,
          series.positiveColor || opts.positiveColor || POSITIVE_COLOR,
          series.negativeColor || opts.negativeColor || NEGATIVE_COLOR);
      } else {
        drawPolyline(graphics, points, tMin, span, vMin, vMax, box,
          series.color || DEFAULT_SERIES_COLOR);
      }
    });

    const hoverValues = [];
    if (typeof pointerX === "number" && pointerX >= box.left && pointerX <= box.left + box.width) {
      const tTarget = tMin + ((pointerX - box.left) / Math.max(1, box.width - 1)) * span;
      graphics.strokeStyle = CROSSHAIR_STROKE;
      graphics.beginPath();
      graphics.moveTo(pointerX, box.top);
      graphics.lineTo(pointerX, box.top + box.height);
      graphics.stroke();
      seriesList.forEach(function (series) {
        const hit = nearestPoint(visiblePoints(series, tMin), tTarget);
        if (!hit) return;
        const color = seriesColor(series, opts, hit.v);
        graphics.fillStyle = color;
        graphics.beginPath();
        graphics.arc(mapX(hit.t, tMin, span, box), mapY(hit.v, vMin, vMax, box),
          CROSSHAIR_RADIUS, 0, Math.PI * 2);
        graphics.fill();
        hoverValues.push({ t: hit.t, v: hit.v, name: series.name || "" });
      });
    }

    graphics.fillStyle = LABEL_FILL;
    graphics.font = LABEL_FONT;
    let label = (opts.legend ? opts.legend + " · " : "") + formatWindowLabel(span);
    if (hoverValues.length) {
      const parts = hoverValues.map(function (item) {
        return (item.name ? item.name + " " : "") +
          formatAxisValue(item.v, opts.yDigits) + (opts.unit ? " " + opts.unit : "");
      });
      label = formatTime(hoverValues[0].t) + " · " + parts.join(" · ");
    }
    graphics.fillText(label, PLOT_LEFT, 12);
  }

  function bindPointer(canvas) {
    if (canvas.dataset.chartBound === "1") return;
    canvas.dataset.chartBound = "1";
    canvas.style.touchAction = "none";
    function redraw(pointerX) {
      const state = chartState.get(canvas);
      if (!state) return;
      state.pointerX = pointerX;
      paintChart(canvas, state.seriesList, state.options, pointerX);
    }
    canvas.addEventListener("pointermove", function (event) {
      const rect = canvas.getBoundingClientRect();
      redraw(((event.clientX - rect.left) / Math.max(1, rect.width)) * canvas.width);
    });
    canvas.addEventListener("pointerleave", function () { redraw(undefined); });
  }

  function drawSeries(canvas, seriesList, options) {
    if (!canvas) return;
    bindPointer(canvas);
    const prev = chartState.get(canvas);
    paintChart(canvas, seriesList, options, prev && prev.pointerX);
    chartState.set(canvas, {
      seriesList: seriesList,
      options: options || {},
      pointerX: prev && prev.pointerX
    });
  }

  global.WattcycleCharts = { drawSeries: drawSeries };
})(window);
