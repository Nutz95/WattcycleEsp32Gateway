/* Lightweight canvas line charts (no external deps). */
(function (global) {
  function syncCanvasSize(canvas) {
    const cssWidth = Math.max(1, Math.floor(canvas.clientWidth || canvas.width || 320));
    const cssHeight = Math.max(1, Math.floor(canvas.clientHeight || 120));
    if (canvas.width !== cssWidth) {
      canvas.width = cssWidth;
    }
    if (canvas.height !== cssHeight) {
      canvas.height = cssHeight;
    }
    return { width: cssWidth, height: cssHeight };
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
    graphics.fillStyle = "rgba(255,255,255,0.03)";
    graphics.fillRect(0, 0, size.width, size.height);

    let min = Infinity;
    let max = -Infinity;
    let pointCount = 0;
    seriesList.forEach(function (series) {
      (series.values || []).forEach(function (value) {
        if (typeof value !== "number" || Number.isNaN(value)) {
          return;
        }
        pointCount += 1;
        if (value < min) min = value;
        if (value > max) max = value;
      });
    });
    if (pointCount === 0 || !isFinite(min) || !isFinite(max)) {
      graphics.fillStyle = "#93a4c7";
      graphics.font = "12px Segoe UI, sans-serif";
      graphics.fillText("Waiting for samples…", 8, 18);
      return;
    }
    if (max === min) {
      max += 1;
      min -= 1;
    }
    const pad = (max - min) * 0.08;
    min -= pad;
    max += pad;

    graphics.strokeStyle = "rgba(255,255,255,0.08)";
    graphics.beginPath();
    for (let i = 1; i < 4; i += 1) {
      const y = (size.height * i) / 4;
      graphics.moveTo(0, y);
      graphics.lineTo(size.width, y);
    }
    graphics.stroke();

    seriesList.forEach(function (series) {
      const values = (series.values || []).filter(function (value) {
        return typeof value === "number" && !Number.isNaN(value);
      });
      if (!values.length) {
        return;
      }
      graphics.strokeStyle = series.color || "#3ecf8e";
      graphics.fillStyle = series.color || "#3ecf8e";
      graphics.lineWidth = 1.6;
      graphics.beginPath();
      values.forEach(function (value, index) {
        const x = values.length === 1
          ? size.width - 2
          : (index / (values.length - 1)) * (size.width - 1);
        const y = size.height - ((value - min) / (max - min)) * (size.height - 1);
        if (index === 0) {
          graphics.moveTo(x, y);
        } else {
          graphics.lineTo(x, y);
        }
      });
      if (values.length === 1) {
        graphics.beginPath();
        const x = size.width - 2;
        const y = size.height - ((values[0] - min) / (max - min)) * (size.height - 1);
        graphics.arc(x, y, 2.5, 0, Math.PI * 2);
        graphics.fill();
      } else {
        graphics.stroke();
      }
    });

    if (options && options.legend) {
      graphics.fillStyle = "#93a4c7";
      graphics.font = "12px Segoe UI, sans-serif";
      graphics.fillText(options.legend, 8, 14);
    }
  }

  global.WattcycleCharts = { drawSeries: drawSeries };
})(window);
