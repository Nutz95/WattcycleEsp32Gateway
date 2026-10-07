/* Time-adaptive canvas charts: x-axis spans sample times, max 24h window. */
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
  const CHART_BG = "rgba(255,255,255,0.03)";
  const GRID_STROKE = "rgba(255,255,255,0.08)";
  const LABEL_FILL = "#93a4c7";
  const DEFAULT_SERIES_COLOR = "#3ecf8e";
  const LABEL_FONT = "12px Segoe UI, sans-serif";
  const LABEL_X = 8;
  const LABEL_Y = 14;
  const LINE_WIDTH = 1.6;
  const SINGLE_POINT_RADIUS = 2.5;

  function syncCanvasSize(canvas) {
    const cssWidth = Math.max(1, Math.floor(canvas.clientWidth || canvas.width || DEFAULT_CANVAS_WIDTH));
    const cssHeight = Math.max(1, Math.floor(canvas.clientHeight || DEFAULT_CANVAS_HEIGHT));
    if (canvas.width !== cssWidth) {
      canvas.width = cssWidth;
    }
    if (canvas.height !== cssHeight) {
      canvas.height = cssHeight;
    }
    return { width: cssWidth, height: cssHeight };
  }

  function collectPoints(seriesList) {
    const points = [];
    seriesList.forEach(function (series) {
      (series.points || []).forEach(function (point) {
        if (typeof point.t !== "number" || typeof point.v !== "number") {
          return;
        }
        if (Number.isNaN(point.t) || Number.isNaN(point.v)) {
          return;
        }
        points.push(point);
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

  function drawSeries(canvas, seriesList, options) {
    if (!canvas) {
      return;
    }
    const size = syncCanvasSize(canvas);
    const graphics = canvas.getContext("2d");
    if (!graphics) {
      return;
    }

    graphics.clearRect(0, 0, size.width, size.height);
    graphics.fillStyle = CHART_BG;
    graphics.fillRect(0, 0, size.width, size.height);

    const allPoints = collectPoints(seriesList);
    if (!allPoints.length) {
      graphics.fillStyle = LABEL_FILL;
      graphics.font = LABEL_FONT;
      graphics.fillText("Waiting for samples…", LABEL_X, LABEL_Y + 4);
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
    for (let i = 1; i <= GRID_LINES; i += 1) {
      const y = (size.height * i) / (GRID_LINES + 1);
      graphics.moveTo(0, y);
      graphics.lineTo(size.width, y);
    }
    graphics.stroke();

    seriesList.forEach(function (series) {
      const points = (series.points || []).filter(function (point) {
        return point.t >= tMin && typeof point.v === "number" && !Number.isNaN(point.v);
      });
      if (!points.length) {
        return;
      }
      const color = series.color || DEFAULT_SERIES_COLOR;
      graphics.strokeStyle = color;
      graphics.fillStyle = color;
      graphics.lineWidth = LINE_WIDTH;
      graphics.beginPath();
      points.forEach(function (point, index) {
        const x = ((point.t - tMin) / span) * (size.width - 1);
        const y = size.height - ((point.v - vMin) / (vMax - vMin)) * (size.height - 1);
        if (index === 0) {
          graphics.moveTo(x, y);
        } else {
          graphics.lineTo(x, y);
        }
      });
      if (points.length === 1) {
        graphics.beginPath();
        const x = ((points[0].t - tMin) / span) * (size.width - 1);
        const y = size.height - ((points[0].v - vMin) / (vMax - vMin)) * (size.height - 1);
        graphics.arc(x, y, SINGLE_POINT_RADIUS, 0, Math.PI * 2);
        graphics.fill();
      } else {
        graphics.stroke();
      }
    });

    graphics.fillStyle = LABEL_FILL;
    graphics.font = LABEL_FONT;
    const label = (options && options.legend)
      ? options.legend + " · " + formatWindowLabel(span)
      : formatWindowLabel(span);
    graphics.fillText(label, LABEL_X, LABEL_Y);
  }

  global.WattcycleCharts = { drawSeries: drawSeries };
})(window);
