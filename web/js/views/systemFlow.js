(function (global) {
  const FLOW_EPS_W = 3;

  function fmtW(value) {
    if (value === undefined || value === null || Number.isNaN(value)) {
      return "-- W";
    }
    return Math.round(Number(value)) + " W";
  }

  function setText(id, text) {
    const el = document.getElementById(id);
    if (el) el.textContent = text;
  }

  function setEdge(id, active, reverse) {
    const el = document.getElementById(id);
    if (!el) return;
    el.classList.toggle("active", !!active);
    el.classList.toggle("reverse", !!reverse);

    const fwd = el.getAttribute("data-path-fwd");
    const rev = el.getAttribute("data-path-rev");
    if (fwd && rev) {
      el.setAttribute("d", reverse ? rev : fwd);
    }

    if (reverse) {
      el.setAttribute("marker-start", "url(#flowArrowRev)");
      el.removeAttribute("marker-end");
    } else {
      el.setAttribute("marker-end", "url(#flowArrow)");
      el.removeAttribute("marker-start");
    }
  }

  function setPortNode(nodeId, portLabelId, on) {
    const node = document.getElementById(nodeId);
    if (node) {
      node.classList.toggle("inactive", !on);
      node.classList.toggle("active", !!on);
    }
    setText(portLabelId, on ? "on" : "off");
  }

  function renderOverview(data) {
    const eco = data.ecoflow || {};
    const solar = data.solar || {};
    const solarLink = !!(data.gateway && data.gateway.solarLink);
    const ecoLink = !!eco.linkFresh || !!(data.gateway && data.gateway.ecoflowLink);

    const summary = document.getElementById("flowSummary");
    if (summary) {
      const parts = [];
      if (data.valid) parts.push("Wattcycle live");
      if (ecoLink && eco.valid) parts.push("EcoFlow live");
      if (solarLink && solar.valid) parts.push("Solar live");
      summary.textContent = parts.length
        ? parts.join(" · ")
        : "Waiting for ESP-NOW / BLE telemetry…";
    }

    const acIn = eco.valid ? eco.acInputW : null;
    setText("lblGridAcIn", fmtW(acIn));
    setEdge("edgeGridEco", eco.valid && acIn > FLOW_EPS_W, false);

    setText("lblEcoSoc", eco.valid && eco.soc != null ? eco.soc + "%" : "--%");
    setText("lblEcoState", ecoLink ? (eco.bleConnected ? "BLE up" : "BLE down") : "no link");

    setText("lblEcoAcOut", fmtW(eco.acOutputW));
    setPortNode("nodeAcOut", "lblAcPort", !!eco.acOutputOn);
    setEdge("edgeEcoAcOut", eco.valid && !!eco.acOutputOn, false);

    setText("lblEcoDcOut", fmtW(eco.dcOutputW));
    setPortNode("nodeDcOut", "lblDcPort", !!eco.dcOutputOn);
    setEdge("edgeEcoDcOut", eco.valid && !!eco.dcOutputOn, false);

    setText("lblEcoUsbOut", fmtW(eco.usbOutputW));
    setPortNode("nodeUsbOut", "lblUsbPort", !!eco.usbOutputOn);
    setEdge("edgeEcoUsbOut", eco.valid && !!eco.usbOutputOn, false);

    // Pack sign convention: + = charging (EcoFlow → Wattcycle), − = discharging (→ EcoFlow).
    const packP = data.valid ? data.power : null;
    setText("lblPackSoc", data.valid ? data.soc + "%" : "--%");
    setText("lblPackPower", fmtW(packP));
    if (packP != null && Math.abs(packP) > FLOW_EPS_W) {
      const packToEco = packP < 0;
      setText("lblPackLink", fmtW(Math.abs(packP)));
      setEdge("edgeEcoPack", true, packToEco);
    } else {
      setText("lblPackLink", "-- W");
      setEdge("edgeEcoPack", false, false);
    }

    const solarP = solar.valid ? solar.power : null;
    setText("lblSolarP", fmtW(solarP));
    const solarActive = solarLink && solar.valid && solarP > FLOW_EPS_W;
    setEdge("edgeSolarPanel", solarActive, false);
    setEdge("edgeXt369p", solarActive, false);
    setEdge("edgeGenasun", solarActive, false);
    setEdge("edgeSolarPack", solarActive, false);
    setText("lblSolarToPack", solarActive ? fmtW(solarP) : "idle");
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.overview = renderOverview;
})(window);
