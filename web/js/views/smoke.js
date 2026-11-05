(function (global) {
  function statusLabel(det) {
    if (det.alarm) {
      return "ALARM";
    }
    if (!det.online) {
      return "stale";
    }
    if (det.mute) {
      return "muted";
    }
    return "ok";
  }

  function batteryLabel(det) {
    if (det.batteryPercent == null || !det.online) {
      return "--%";
    }
    return Math.round(det.batteryPercent) + "%";
  }

  function paintMetrics(bubble, det, title) {
    bubble.querySelector(".plug-bubble-title").textContent = title;
    bubble.querySelector(".plug-bubble-id").textContent = det.id;
    bubble.querySelector(".plug-bubble-power").textContent = statusLabel(det);
    const metrics = bubble.querySelector(".plug-bubble-metrics");
    metrics.innerHTML = '<span class="smoke-battery"></span><span class="smoke-mute"></span>';
    metrics.querySelector(".smoke-battery").textContent = "batt " + batteryLabel(det);
    metrics.querySelector(".smoke-mute").textContent = det.mute ? "muted" : "";
    bubble.classList.toggle("is-offline", !det.online);
    bubble.classList.toggle("is-alarm", !!det.alarm);
  }

  function paintOverview(list) {
    const overviewHost = document.getElementById("smokeOverview");
    if (!overviewHost) {
      return;
    }
    const dets = list || [];
    if (dets.length === 0) {
      overviewHost.hidden = true;
      overviewHost.replaceChildren();
      return;
    }
    overviewHost.hidden = false;
    overviewHost.replaceChildren();
    const title = document.createElement("p");
    title.className = "smoke-overview-title";
    title.textContent = "Smoke";
    overviewHost.appendChild(title);
    const row = document.createElement("div");
    row.className = "smoke-overview-row";
    dets.forEach(function (det) {
      const chip = document.createElement("button");
      chip.type = "button";
      chip.className = "smoke-chip";
      chip.classList.toggle("is-alarm", !!det.alarm);
      chip.classList.toggle("is-offline", !det.online);
      chip.textContent =
        (det.name || det.id) + " · " + statusLabel(det) + " · " + batteryLabel(det);
      chip.addEventListener("click", function () {
        const tab = document.querySelector('.tab[data-tab="smoke"]');
        if (tab) {
          tab.click();
        }
      });
      row.appendChild(chip);
    });
    overviewHost.appendChild(row);
  }

  const admin = global.WattcycleShellyDeviceAdmin.create({
    listenField: "smoke.listen",
    nameFieldPrefix: "smoke.name.",
    dataAttr: "data-smoke-id",
    apiBase: "/api/smoke",
    bubbleClass: "plug-bubble smoke-bubble",
    paintMetrics: paintMetrics,
    emptyAuthorized: "None yet. Enable listen, then authorize a posting detector.",
    emptyPostingOpen: "Waiting for UDP frames (press the smoke button to wake)…",
    emptyPostingClosed: "Turn on listen to discover unknown detectors.",
    unavailableText: "Smoke API unavailable on this hub.",
    afterRefresh: function (body) {
      if (!body) {
        paintOverview([]);
        return;
      }
      paintOverview(body.authorized || []);
    },
  });

  function bind() {
    admin.bind({
      listen: "smokeListen",
      authorized: "smokeAuthorized",
      posting: "smokePosting",
    });
  }

  global.WattcycleSmoke = { bind: bind, refresh: admin.refresh, refreshOverview: admin.refresh };
  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.smoke = admin.refresh;
})(window);
