(function (global) {
  let deviceInfo = null;
  let deviceFetchStarted = false;
  let lastBleAddress = "";

  function setLink(id, ok, label) {
    const el = document.getElementById(id);
    if (!el) return;
    el.textContent = label || (ok ? "ok" : "down");
    el.className = ok ? "ok" : "bad";
  }

  async function ensureDeviceInfo() {
    if (deviceInfo || deviceFetchStarted) return;
    deviceFetchStarted = true;
    try {
      const res = await fetch("/api/device", { credentials: "same-origin" });
      if (res.ok) {
        deviceInfo = await res.json();
        global.WattcycleDevice = deviceInfo;
      }
    } catch (err) {
      deviceFetchStarted = false;
    }
  }

  function renderGateway(data) {
    ensureDeviceInfo();
    const gateway = data.gateway || {};
    const product = data.product || {};
    const solar = data.solar || {};

    setLink("wifi", gateway.wifi, gateway.ip || (gateway.wifi ? "ok" : "down"));
    document.getElementById("webPort").textContent = String(gateway.webPort || 6789);
    document.getElementById("hubError").textContent = gateway.error || "none";

    const ntpEl = document.getElementById("gatewayNtp");
    if (ntpEl) {
      const ntpOk = !!(gateway.ntp || (deviceInfo && deviceInfo.ntp));
      ntpEl.textContent = (gateway.ntp !== undefined || deviceInfo)
        ? (ntpOk ? "synced" : "pending")
        : "--";
      ntpEl.className = ntpOk ? "ok" : "bad";
    }
    const ecoflowMacEl = document.getElementById("ecoflowBridgeMac");
    if (deviceInfo) {
      document.getElementById("hubStaMac").textContent = deviceInfo.hubStaMac || "--";
      document.getElementById("bmsBridgeMac").textContent = deviceInfo.bmsBridgeMac || "--";
      document.getElementById("xtBridgeMac").textContent = deviceInfo.xtBridgeMac || "--";
      if (ecoflowMacEl) {
        ecoflowMacEl.textContent = deviceInfo.ecoflowBridgeMac || "--";
      }
    } else if (ecoflowMacEl) {
      ecoflowMacEl.textContent = "--";
    }

    setLink("ble", gateway.ble, gateway.ble ? "connected" : "down");
    const bleAddress = gateway.bleAddress || "";
    if (bleAddress) {
      lastBleAddress = bleAddress;
    }
    document.getElementById("bleAddress").textContent = bleAddress || lastBleAddress || "--";
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

    const eco = data.ecoflow || {};
    const ecoLink = !!(gateway.ecoflowLink || eco.linkFresh);
    setLink("ecoflowLinkGw", ecoLink, ecoLink ? "fresh" : "stale/down");
    const ecoEncrypted = !!eco.espNowEncrypted;
    const ecoEncLabel = ecoLink || ecoEncrypted
      ? (ecoEncrypted ? "secured (PMK)" : "plaintext")
      : "--";
    setLink("ecoflowEncGw", ecoEncrypted, ecoEncLabel);
    if (!ecoEncrypted && ecoLink) {
      const encEl = document.getElementById("ecoflowEncGw");
      if (encEl) encEl.className = "warn";
    }
    setLink("ecoflowBleGw", eco.bleConnected, eco.bleConnected ? "connected" : "down");
    const ecoErr = document.getElementById("ecoflowError");
    if (ecoErr) ecoErr.textContent = eco.error || "none";
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.gateway = renderGateway;
})(window);
