(function (global) {
  const COLORS = ["#3ecf8e", "#f0b429", "#60a5fa", "#c084fc", "#fb7185", "#38bdf8", "#a3e635", "#f97316"];

  function drawPie(canvas, slices, options) {
    const opts = options || {};
    if (!canvas) return;
    const cssWidth = Math.max(1, Math.floor(canvas.clientWidth || canvas.width || 280));
    const cssHeight = Math.max(1, Math.floor(canvas.clientHeight || 180));
    canvas.width = cssWidth;
    canvas.height = cssHeight;
    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, cssWidth, cssHeight);
    const total = slices.reduce(function (sum, slice) {
      return sum + (Number(slice.value) > 0 ? Number(slice.value) : 0);
    }, 0);
    const cx = cssWidth * 0.35;
    const cy = cssHeight * 0.5;
    const radius = Math.min(cx, cy) - 8;
    if (!(total > 0) || radius <= 0) {
      ctx.fillStyle = "#93a4c7";
      ctx.font = "12px Segoe UI, sans-serif";
      ctx.fillText(opts.emptyLabel || "No data", 12, cssHeight / 2);
      return;
    }
    let angle = -Math.PI / 2;
    slices.forEach(function (slice, index) {
      const value = Number(slice.value);
      if (!(value > 0)) return;
      const sweep = (value / total) * Math.PI * 2;
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.arc(cx, cy, radius, angle, angle + sweep);
      ctx.closePath();
      ctx.fillStyle = slice.color || COLORS[index % COLORS.length];
      ctx.fill();
      angle += sweep;
    });
    let legendY = 16;
    ctx.font = "11px Segoe UI, sans-serif";
    slices.forEach(function (slice, index) {
      const value = Number(slice.value);
      if (!(value > 0)) return;
      const color = slice.color || COLORS[index % COLORS.length];
      ctx.fillStyle = color;
      ctx.fillRect(cssWidth * 0.62, legendY - 8, 10, 10);
      ctx.fillStyle = "#e8eefc";
      const label =
        (slice.label || "?") +
        " " +
        value.toFixed(opts.digits == null ? 1 : opts.digits) +
        (opts.unit || "");
      ctx.fillText(label, cssWidth * 0.62 + 16, legendY);
      legendY += 16;
    });
  }

  global.WattcycleReportsPie = { drawPie: drawPie, COLORS: COLORS };
})(window);
