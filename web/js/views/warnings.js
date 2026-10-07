(function (global) {
  function hex(value) {
    return "0x" + (value >>> 0).toString(16).padStart(2, "0");
  }

  function renderWarnings(data) {
    const warnings = data.warnings || {};
    const protection = document.getElementById("protectionFlag");
    const fault = document.getElementById("faultFlag");
    protection.textContent = warnings.protection ? "yes" : "no";
    protection.className = warnings.protection ? "bad" : "ok";
    fault.textContent = warnings.fault ? "yes" : "no";
    fault.className = warnings.fault ? "bad" : "ok";
    document.getElementById("status1").textContent = hex(warnings.status1 || 0);
    document.getElementById("status2").textContent = hex(warnings.status2 || 0);
    document.getElementById("status5").textContent = hex(warnings.status5 || 0);
    document.getElementById("warn1").textContent = hex(warnings.warn1 || 0);
    document.getElementById("warn2").textContent = hex(warnings.warn2 || 0);
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.warnings = renderWarnings;
})(window);
