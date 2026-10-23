(function () {
  let refreshTimer = null;

  function activateTab(name) {
    const target = document.querySelector('.tab[data-tab="' + name + '"]');
    if (target && target.hidden) {
      return;
    }
    document.querySelectorAll(".tab").forEach(function (tab) {
      tab.classList.toggle("active", tab.getAttribute("data-tab") === name);
    });
    document.querySelectorAll(".view").forEach(function (view) {
      view.hidden = view.getAttribute("data-view") !== name;
    });
    if (window.WattcycleCamera) {
      window.WattcycleCamera.setActive(name === "camera");
    }
    if (name === "account" && window.WattcycleViews && window.WattcycleViews.reloadAccount) {
      window.WattcycleViews.reloadAccount();
    }
    if (name === "plugs" && window.WattcyclePlugs) {
      window.WattcyclePlugs.bind();
    }
    if (name === "camera") {
      if (refreshTimer) {
        clearInterval(refreshTimer);
        refreshTimer = null;
      }
    } else if (!refreshTimer && document.getElementById("appShell") &&
               !document.getElementById("appShell").hidden) {
      refresh();
      refreshTimer = setInterval(refresh, 2000);
    }
  }

  document.querySelectorAll(".tab").forEach(function (tab) {
    tab.addEventListener("click", function () {
      activateTab(tab.getAttribute("data-tab"));
    });
  });

  function gotoFromNode(event) {
    const node = event.target.closest("[data-goto]");
    if (!node || !node.closest("#views")) return;
    activateTab(node.getAttribute("data-goto"));
  }

  document.addEventListener("click", gotoFromNode);
  document.addEventListener("keydown", function (event) {
    if (event.key !== "Enter" && event.key !== " ") return;
    const node = event.target.closest("[data-goto]");
    if (!node) return;
    event.preventDefault();
    activateTab(node.getAttribute("data-goto"));
  });

  async function refresh() {
    try {
      const response = await fetch("/api/telemetry.bin", {
        cache: "no-store",
        credentials: "same-origin"
      });
      if (response.status === 401) {
        if (refreshTimer) {
          clearInterval(refreshTimer);
          refreshTimer = null;
        }
        window.WattcycleAuth.handleUnauthorized();
        return;
      }
      if (!response.ok) {
        throw new Error("HTTP " + response.status);
      }
      const buffer = await response.arrayBuffer();
      const data = window.WattcycleBinary.decodeTelemetry(buffer);
      const packHistory = data.valid
        ? window.WattcycleHistory.pushSample({
            t: Date.now(),
            v: data.voltage,
            i: data.current,
            p: data.power,
            ah: data.remainingAh,
            mos: data.mosTemp,
            pcb: data.pcbTemp
          })
        : window.WattcycleHistory.all();

      const solar = data.solar || {};
      const solarHistory =
        data.gateway && data.gateway.solarLink && solar.valid
          ? window.WattcycleSolarHistory.pushSample({
              t: Date.now(),
              v: solar.voltage,
              i: solar.current,
              p: solar.power,
              wh: solar.energyWh,
              ah: solar.capacityAh
            })
          : window.WattcycleSolarHistory.all();

      const eco = data.ecoflow || {};
      const ecoflowHistory =
        eco.linkFresh && eco.bleConnected && eco.valid
          ? window.WattcycleEcoflowHistory.pushSample({
              t: Date.now(),
              soc: eco.soc,
              acOut: eco.acOutputW,
              acIn: eco.acInputW,
              dcOut: eco.dcOutputW,
              usbOut: eco.usbOutputW
            })
          : window.WattcycleEcoflowHistory.all();

      window.WattcycleViews.overview(data);
      window.WattcycleViews.dashboard(data);
      window.WattcycleViews.solar(data, solarHistory);
      window.WattcycleViews.wattcycle(data, packHistory);
      window.WattcycleViews.ecoflow(data);
      window.WattcycleViews.history(data, packHistory, solarHistory, ecoflowHistory);
      window.WattcycleViews.cells(data);
      window.WattcycleViews.temperatures(data);
      window.WattcycleViews.warnings(data);
      window.WattcycleViews.gateway(data);
      window.WattcycleViews.esp(data);
      window.WattcycleViews.account();
    } catch (error) {
      const summary = document.getElementById("summary");
      if (summary) {
        summary.textContent = "API unreachable: " + error.message;
      }
    }
  }

  const guestTabNames = window.WattcycleGuestTabs || [];

  function applyGuest(active, mask) {
    const banner = document.getElementById("guestBanner");
    if (banner) {
      banner.hidden = !active;
    }
    document.querySelectorAll(".tab").forEach(function (tab) {
      const name = tab.getAttribute("data-tab");
      if (!active || name === "account") {
        tab.hidden = false;
        return;
      }
      if (name === "camera") {
        tab.hidden = true;
        return;
      }
      const index = guestTabNames.indexOf(name);
      tab.hidden = index < 0 || ((mask >> index) & 1) === 0;
    });
    if (active) {
      const current = document.querySelector(".tab.active");
      if (!current || current.hidden) {
        const visible = document.querySelector(".tab:not([hidden])");
        if (visible) {
          activateTab(visible.getAttribute("data-tab"));
        }
      }
    }
  }

  async function refreshGuest() {
    if (!window.WattcycleAuth || !window.WattcycleAuth.guest) {
      applyGuest(false, 0);
      return;
    }
    let mask = 0;
    try {
      const response = await fetch("/api/guest", { cache: "no-store", credentials: "same-origin" });
      if (response.ok) {
        const payload = await response.json();
        if (typeof payload.tabs === "number") {
          mask = payload.tabs;
        }
      }
    } catch (error) {
      mask = 0;
    }
    applyGuest(true, mask);
  }

  window.WattcycleGuest = { refresh: refreshGuest };

  function startDashboard() {
    window.WattcycleAuth.showApp();
    if (!window.WattcycleAuth.guest) {
      activateTab("overview");
    }
    refreshGuest();
    refresh();
    if (refreshTimer) {
      clearInterval(refreshTimer);
    }
    refreshTimer = setInterval(refresh, 2000);
  }

  window.WattcycleAuth.bootstrap(startDashboard);
})();
