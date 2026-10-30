(function (global) {
  const COLORS = ["#3ecf8e", "#f0b429", "#60a5fa", "#c084fc", "#fb7185", "#38bdf8"];

  function drawCategoryBars(canvas, categories, options) {
    const opts = options || {};
    if (!canvas) return;
    const cssWidth = Math.max(1, Math.floor(canvas.clientWidth || canvas.width || 320));
    const cssHeight = Math.max(1, Math.floor(canvas.clientHeight || 160));
    canvas.width = cssWidth;
    canvas.height = cssHeight;
    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, cssWidth, cssHeight);
    const left = 44;
    const right = 8;
    const top = 18;
    const bottom = 36;
    const plotW = Math.max(1, cssWidth - left - right);
    const plotH = Math.max(1, cssHeight - top - bottom);
    const bands = opts.bands || ["total"];
    let max = 0;
    (categories || []).forEach(function (cat) {
      let sum = 0;
      bands.forEach(function (band) {
        sum += Number((cat.values && cat.values[band]) || 0);
      });
      if (sum > max) max = sum;
    });
    if (!(max > 0) || !(categories || []).length) {
      ctx.fillStyle = "#93a4c7";
      ctx.font = "12px Segoe UI, sans-serif";
      ctx.fillText(opts.emptyLabel || "No data", 12, cssHeight / 2);
      return;
    }
    ctx.strokeStyle = "rgba(255,255,255,0.08)";
    ctx.beginPath();
    for (let g = 0; g <= 3; g += 1) {
      const y = top + (plotH * g) / 3;
      ctx.moveTo(left, y);
      ctx.lineTo(left + plotW, y);
    }
    ctx.stroke();
    ctx.fillStyle = "#93a4c7";
    ctx.font = "11px Segoe UI, sans-serif";
    ctx.textAlign = "right";
    ctx.textBaseline = "middle";
    for (let g = 0; g <= 3; g += 1) {
      const value = max * (1 - g / 3);
      const y = top + (plotH * g) / 3;
      ctx.fillText(value >= 1000 ? (value / 1000).toFixed(1) + "k" : value.toFixed(0), left - 6, y);
    }
    const n = categories.length;
    const slot = plotW / n;
    const barW = Math.max(6, Math.min(42, slot * 0.62));
    categories.forEach(function (cat, index) {
      const xCenter = left + slot * index + slot / 2;
      let y = top + plotH;
      bands.forEach(function (band, bandIndex) {
        const value = Number((cat.values && cat.values[band]) || 0);
        if (!(value > 0)) return;
        const h = (value / max) * plotH;
        y -= h;
        ctx.fillStyle = (opts.bandColors && opts.bandColors[band]) || COLORS[bandIndex % COLORS.length];
        ctx.fillRect(xCenter - barW / 2, y, barW, h);
      });
      ctx.fillStyle = "#e8eefc";
      ctx.textAlign = "center";
      ctx.textBaseline = "top";
      const label = String(cat.label || "").slice(0, 12);
      ctx.fillText(label, xCenter, top + plotH + 6);
    });
    if (opts.legend) {
      ctx.textAlign = "left";
      ctx.textBaseline = "top";
      let lx = left;
      bands.forEach(function (band, bandIndex) {
        const color = (opts.bandColors && opts.bandColors[band]) || COLORS[bandIndex % COLORS.length];
        ctx.fillStyle = color;
        ctx.fillRect(lx, 2, 10, 10);
        ctx.fillStyle = "#93a4c7";
        ctx.fillText(band, lx + 14, 1);
        lx += ctx.measureText(band).width + 28;
      });
    }
  }

  global.WattcycleReportsBars = { drawCategoryBars: drawCategoryBars };
})(window);
