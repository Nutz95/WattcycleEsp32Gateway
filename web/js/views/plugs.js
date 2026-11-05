(function (global) {
  function wattsLabel(plug) {
    if (!plug.online) {
      return "offline";
    }
    return Math.round(plug.watts) + " W";
  }

  function todayLabel(plug) {
    if (plug.whToday == null) {
      return "-- Wh today";
    }
    return plug.whToday.toFixed(1) + " Wh today";
  }

  function paintMetrics(bubble, plug, title) {
    bubble.querySelector(".plug-bubble-title").textContent = title;
    bubble.querySelector(".plug-bubble-id").textContent = plug.id;
    bubble.querySelector(".plug-bubble-power").textContent = wattsLabel(plug);
    const metrics = bubble.querySelector(".plug-bubble-metrics");
    metrics.innerHTML =
      '<span class="plug-bubble-today"></span><span class="plug-bubble-relay"></span>';
    metrics.querySelector(".plug-bubble-today").textContent = todayLabel(plug);
    metrics.querySelector(".plug-bubble-relay").textContent = plug.output ? "ON" : "OFF";
    bubble.classList.toggle("is-offline", !plug.online);
    bubble.classList.toggle("is-on", !!plug.output && !!plug.online);
  }

  function gotoHistory(plugId) {
    const tab = document.querySelector('.tab[data-tab="history"]');
    if (tab) {
      tab.click();
    }
    if (global.WattcycleHistoryPlugs && global.WattcycleHistoryPlugs.focusPlug) {
      global.WattcycleHistoryPlugs.focusPlug(plugId);
    }
  }

  const admin = global.WattcycleShellyDeviceAdmin.create({
    listenField: "plug.listen",
    nameFieldPrefix: "plug.name.",
    dataAttr: "data-plug-id",
    apiBase: "/api/plugs",
    paintMetrics: paintMetrics,
    emptyAuthorized: "None yet. Enable listen, then authorize a posting plug.",
    emptyPostingOpen: "Waiting for UDP frames...",
    emptyPostingClosed: "Turn on listen to discover unknown plugs.",
    unavailableText: "Plugs API unavailable on this hub.",
    onAuthorizedActions: function (actions, plug) {
      const histBtn = document.createElement("button");
      histBtn.type = "button";
      histBtn.className = "ghost-btn";
      histBtn.textContent = "History";
      histBtn.addEventListener("click", function () {
        gotoHistory(plug.id);
      });
      actions.appendChild(histBtn);
    },
  });

  function bind() {
    admin.bind({
      listen: "plugsListen",
      authorized: "plugsAuthorized",
      posting: "plugsPosting",
    });
  }

  global.WattcyclePlugs = { bind: bind, refresh: admin.refresh };
  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.plugs = admin.refresh;
})(window);
