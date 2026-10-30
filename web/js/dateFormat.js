(function (global) {
  /// Format API day key YYYY-MM-DD as dd/mm/yyyy for display.
  function ymdToDmy(ymd) {
    const text = String(ymd || "");
    if (!/^\d{4}-\d{2}-\d{2}$/.test(text)) {
      return text;
    }
    return text.slice(8, 10) + "/" + text.slice(5, 7) + "/" + text.slice(0, 4);
  }

  global.WattcycleDateFormat = { ymdToDmy: ymdToDmy };
})(window);
