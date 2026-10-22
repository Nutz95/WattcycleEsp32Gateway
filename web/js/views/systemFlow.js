(function (global) {
  // Pack / Eco / grid: ignore BMS noise under a few watts.
  const FLOW_EPS_W = 3;
  // Solar XT369P: user tracks production from 1 W upward.
  const SOLAR_EPS_W = 1;

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

  function formatRemain(minutes) {
    const total = Number(minutes);
    if (!Number.isFinite(total) || total <= 0) {
      return "";
    }
    if (total >= 60) {
      const hours = Math.floor(total / 60);
      const mins = total % 60;
      return hours + "h" + String(mins).padStart(2, "0");
    }
    return total + "min";
  }

  function setEcoHeadline(eco) {
    const el = document.getElementById("lblEcoSoc");
    if (!el) return;
    el.replaceChildren();
    if (!eco.valid || eco.soc == null) {
      el.textContent = "--%";
      return;
    }
    const remainLabel = formatRemain(eco.remainMinutes);
    el.textContent = remainLabel ? eco.soc + "%  " + remainLabel : eco.soc + "%";
  }

  function setEcoBatteryLine(eco, ecoLink) {
    const el = document.getElementById("lblEcoState");
    if (!el) return;
    el.classList.remove("flow-batt-neg", "flow-batt-pos");
    const watts = Number(eco.bmsPowerW);
    if (eco.valid && Number.isFinite(watts) && Math.abs(watts) > FLOW_EPS_W) {
      el.textContent = "batt " + (watts > 0 ? "+" : "") + Math.round(watts) + "W";
      el.classList.add(watts < 0 ? "flow-batt-neg" : "flow-batt-pos");
      return;
    }
    el.textContent = ecoLink ? (eco.bleConnected ? "BLE up" : "BLE down") : "no link";
  }

  /** Station own use: port gap minus signed battery power. Positive watts only. */
  function stationSelfW(balance, batteryW) {
    if (balance == null || !Number.isFinite(batteryW) || Math.abs(batteryW) <= FLOW_EPS_W) {
      return null;
    }
    const self = balance - batteryW;
    if (self <= FLOW_EPS_W) return null;
    return self;
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

  function setLinkNode(id, up) {
    const node = document.getElementById(id);
    if (!node) return;
    node.classList.toggle("link-up", !!up);
    node.classList.toggle("link-down", !up);
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
    const gw = data.gateway || {};
    const bmsLink = !!gw.telemetryFresh;
    const solarLink = !!gw.solarLink;
    const ecoLink = !!eco.linkFresh || !!gw.ecoflowLink;

    const summary = document.getElementById("flowSummary");
    if (summary) {
      const parts = [];
      if (bmsLink && data.valid) parts.push("Wattcycle live");
      if (ecoLink && eco.valid) parts.push("EcoFlow live");
      if (solarLink && solar.valid) parts.push("Solar live");
      summary.textContent = parts.length
        ? parts.join(" · ")
        : "Waiting for ESP-NOW / BLE telemetry…";
    }

    const acIn = eco.valid ? eco.acInputW : null;
    setText("lblGridAcIn", fmtW(acIn));
    setEdge("edgeGridEco", eco.valid && acIn > FLOW_EPS_W, false);

    setEcoHeadline(eco);
    const ecoBleUp = ecoLink && !!eco.bleConnected;
    setEcoBatteryLine(eco, ecoLink);
    setLinkNode("nodeEco", ecoBleUp);

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
    const packBleUp = bmsLink && !!gw.ble;
    setText("lblPackState", bmsLink ? (gw.ble ? "BLE up" : "BLE down") : "no link");
    setLinkNode("nodePack", packBleUp);
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
          ? "~" + watts + " W in−out"
          : "~" + watts + " W out−in";
        balEl.classList.toggle("flow-balance-pos", balance > 0);
        balEl.classList.toggle("flow-balance-neg", balance < 0);
      } else {
        balEl.textContent = "";
        balEl.classList.remove("flow-balance-pos", "flow-balance-neg");
      }
    }
    const selfEl = document.getElementById("lblEcoSelf");
    if (selfEl) {
      const batteryW = eco.valid ? Number(eco.bmsPowerW) : NaN;
      const self = stationSelfW(balance, batteryW);
      selfEl.textContent = self == null ? "" : "~" + Math.round(self) + " W self";
    }

    const solarP = solar.valid ? solar.power : null;
    setText("lblSolarP", fmtW(solarP));
    const sppOk = !!(gw.spp || solar.sppConnected);
    setText("lblXtState", solarLink ? (sppOk ? "SPP up" : "SPP down") : "no link");
    setLinkNode("nodeXt", solarLink && sppOk);
    const solarActive = solarLink && solar.valid && solarP > SOLAR_EPS_W;
    const packCharging = packP != null && packP > FLOW_EPS_W;
    const solarToPack = solarActive && packCharging;
    const solarToEco = solarActive && !packCharging;
    setEdge("edgeSolarPanel", solarActive, false);
    setEdge("edgeXt369p", solarActive, false);
    setEdge("edgeGenasun", solarToPack, false);
    setEdge("edgeSolarPack", solarToPack, false);
    setEdge("edgeGenasunEco", solarToEco, false);
    setText("lblSolarToPack", solarToPack ? fmtW(solarP) : (solarToEco ? "" : "idle"));
    setText("lblSolarToEco", solarToEco ? fmtW(solarP) : "");
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.overview = renderOverview;
})(window);
