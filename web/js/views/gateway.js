(function (global) {
  function setLink(id, ok, label) {
    const el = document.getElementById(id);
    el.textContent = label || (ok ? "ok" : "down");
    el.className = ok ? "ok" : "bad";
  }

  function renderGateway(data) {
    const gateway = data.gateway || {};
    const product = data.product || {};
    setLink("wifi", gateway.wifi, gateway.ip || (gateway.wifi ? "ok" : "down"));
    setLink("ble", gateway.ble, gateway.ble ? "connected" : "down");
    document.getElementById("bleAddress").textContent = gateway.bleAddress || "--";
    setLink("solarLink", gateway.solarLink, gateway.solarLink ? "fresh" : "stale/down");
    setLink("solarSppGw", gateway.spp, gateway.spp ? "connected" : "down");
    const solar = data.solar || {};
    document.getElementById("solarTarget").textContent = solar.target || "--";
    document.getElementById("error").textContent =
      solar.error || gateway.error || "none";
    document.getElementById("fw").textContent = product.fw || "--";
    document.getElementById("mfr").textContent = product.mfr || "--";
    document.getElementById("sn").textContent = product.sn || "--";
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.gateway = renderGateway;
})(window);
