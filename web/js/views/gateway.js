(function (global) {
  function setLink(id, ok, label) {
    const el = document.getElementById(id);
    if (!el) return;
    el.textContent = label || (ok ? "ok" : "down");
    el.className = ok ? "ok" : "bad";
  }

  function renderGateway(data) {
    const gateway = data.gateway || {};
    const product = data.product || {};
    const solar = data.solar || {};

    setLink("wifi", gateway.wifi, gateway.ip || (gateway.wifi ? "ok" : "down"));
    document.getElementById("webPort").textContent = "6789";
    document.getElementById("hubError").textContent = gateway.error || "none";

    setLink("ble", gateway.ble, gateway.ble ? "connected" : "down");
    document.getElementById("bleAddress").textContent = gateway.bleAddress || "--";
    document.getElementById("fw").textContent = product.fw || "--";
    document.getElementById("mfr").textContent = product.mfr || "--";
    document.getElementById("sn").textContent = product.sn || "--";

    setLink("solarLink", gateway.solarLink, gateway.solarLink ? "fresh" : "stale/down");
    const encrypted = !!(solar.espNowEncrypted || gateway.espNowEncrypted);
    const encLabel = gateway.solarLink || encrypted
      ? (encrypted ? "secured (PMK)" : "plaintext")
      : "--";
    setLink("espNowEnc", encrypted, encLabel);
    if (!encrypted && (gateway.solarLink || solar.target)) {
      document.getElementById("espNowEnc").className = "warn";
    }
    setLink("solarSppGw", gateway.spp, gateway.spp ? "connected" : "down");
    document.getElementById("solarTarget").textContent = solar.target || "--";
    document.getElementById("solarError").textContent = solar.error || "none";
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.gateway = renderGateway;
})(window);
