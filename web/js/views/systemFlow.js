(function (global) {
  const FLOW_EPS_W = 3;

  function fmtW(value) {
    if (value === undefined || value === null || Number.isNaN(value)) {
      return "-- W";
    }
    return Math.round(Number(value)) + " W";
  }

  function num(value) {
    const n = Number(value);
    return Number.isFinite(n) ? n : 0;
  }

  function setText(id, text) {
    const el = document.getElementById(id);
    if (el) el.textContent = text;
  }

  function setEdge(id, active, reverse) {
    const el = document.getElementById(id);
    if (!el) return;
    el.classList.toggle("active", !!active);

    const fwd = el.getAttribute("data-path-fwd");
    const rev = el.getAttribute("data-path-rev");
    if (fwd && rev) {
      // Bidirectional spine: encode direction in `d` and keep marker-end on the
      // downstream tip (reverse path + reverse marker was canceling out).
      el.setAttribute("d", reverse ? rev : fwd);
      el.classList.remove("reverse");
    } else {
      el.classList.toggle("reverse", !!reverse);
    }

    el.setAttribute("marker-end", "url(#flowArrow)");
    el.removeAttribute("marker-start");
  }

  function setPortNode(nodeId, portLabelId, on) {
    const node = document.getElementById(nodeId);
    if (node) {
      node.classList.toggle("inactive", !on);
      node.classList.toggle("active", !!on);
    }
    setText(portLabelId, on ? "on" : "off");
  }

  /** EcoFlow retained power: inputs − outputs (+ = charge/self, − = from batt). */
  function ecoBalanceW(eco, packP) {
    if (!eco.valid) return null;
    const acIn = num(eco.acInputW);
    const solarIn = num(eco.solarInputW);
    const outs = num(eco.acOutputW) + num(eco.dcOutputW) + num(eco.usbOutputW);
    let packIn = 0;
    let packOut = 0;
    if (packP != null && Math.abs(packP) > FLOW_EPS_W) {
      if (packP < 0) packIn = Math.abs(packP);
      else packOut = packP;
    }
    return acIn + solarIn + packIn - outs - packOut;
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

    const balance = ecoBalanceW(eco, packP);
    const balEl = document.getElementById("lblEcoBalance");
    if (balEl) {
      if (balance != null && Math.abs(balance) > FLOW_EPS_W) {
        const watts = Math.round(Math.abs(balance));
        balEl.textContent = balance > 0
          ? "~" + watts + " W charge/self"
          : "~" + watts + " W from batt";
        balEl.classList.toggle("flow-balance-pos", balance > 0);
        balEl.classList.toggle("flow-balance-neg", balance < 0);
      } else {
        balEl.textContent = "";
        balEl.classList.remove("flow-balance-pos", "flow-balance-neg");
      }
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
