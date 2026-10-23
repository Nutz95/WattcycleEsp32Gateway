(function (global) {
  const drafts = new global.WattcycleDraftStore();
  const LISTEN_FIELD = "plug.listen";
  let listenBox = null;
  let authorizedHost = null;
  let postingHost = null;
  let timer = null;
  let bound = false;

  function postJson(url, body) {
    return fetch(url, {
      method: "POST",
      credentials: "same-origin",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
    }).then(function (res) {
      if (!res.ok) {
        throw new Error("request failed");
      }
      return res.json().catch(function () {
        return {};
      });
    });
  }

  function fieldId(plugId) {
    return "plug.name." + plugId;
  }

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

  function ensureBubble(host, plugId) {
    let bubble = host.querySelector('[data-plug-id="' + plugId + '"]');
    if (bubble) {
      return bubble;
    }
    bubble = document.createElement("article");
    bubble.className = "plug-bubble";
    bubble.setAttribute("data-plug-id", plugId);
    bubble.innerHTML =
      '<div class="plug-bubble-head">' +
      '<p class="plug-bubble-title"></p>' +
      '<p class="plug-bubble-id hint"></p>' +
      "</div>" +
      '<p class="plug-bubble-power"></p>' +
      '<div class="plug-bubble-metrics">' +
      '<span class="plug-bubble-today"></span>' +
      '<span class="plug-bubble-relay"></span>' +
      "</div>" +
      '<div class="plug-bubble-edit"></div>' +
      '<div class="plug-bubble-actions"></div>';
    host.appendChild(bubble);
    return bubble;
  }

  function pruneHost(host, keepIds, emptyText) {
    Array.prototype.slice.call(host.children).forEach(function (child) {
      const id = child.getAttribute("data-plug-id");
      if (!id || keepIds.indexOf(id) < 0) {
        child.remove();
      }
    });
    const empty = host.querySelector("[data-empty]");
    if (keepIds.length === 0) {
      if (!empty) {
        const note = document.createElement("p");
        note.className = "hint";
        note.setAttribute("data-empty", "1");
        note.textContent = emptyText;
        host.appendChild(note);
      } else {
        empty.textContent = emptyText;
      }
    } else if (empty) {
      empty.remove();
    }
  }

  function paintMetrics(bubble, plug, title) {
    bubble.querySelector(".plug-bubble-title").textContent = title;
    bubble.querySelector(".plug-bubble-id").textContent = plug.id;
    bubble.querySelector(".plug-bubble-power").textContent = wattsLabel(plug);
    bubble.querySelector(".plug-bubble-today").textContent = todayLabel(plug);
    bubble.querySelector(".plug-bubble-relay").textContent = plug.output ? "ON" : "OFF";
    bubble.classList.toggle("is-offline", !plug.online);
    bubble.classList.toggle("is-on", !!plug.output && !!plug.online);
  }

  function bindNameInput(bubble, plugId, serverName) {
    const edit = bubble.querySelector(".plug-bubble-edit");
    let input = edit.querySelector("input");
    if (!input) {
      input = document.createElement("input");
      input.type = "text";
      input.maxLength = 24;
      input.placeholder = "Display name";
      input.addEventListener("input", function () {
        drafts.beginEdit(fieldId(plugId), input.value);
      });
      edit.appendChild(input);
    }
    if (document.activeElement !== input) {
      input.value = drafts.overlay(fieldId(plugId), serverName || "");
    }
    return input;
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

  function paintAuthorizedBubble(plug) {
    const bubble = ensureBubble(authorizedHost, plug.id);
    paintMetrics(bubble, plug, plug.name || plug.id);
    const input = bindNameInput(bubble, plug.id, plug.name || "");
    const actions = bubble.querySelector(".plug-bubble-actions");
    if (actions.getAttribute("data-bound")) {
      return;
    }
    actions.setAttribute("data-bound", "1");
    const renameBtn = document.createElement("button");
    renameBtn.type = "button";
    renameBtn.textContent = "Rename";
    renameBtn.addEventListener("click", function () {
      const name = input.value;
      postJson("/api/plugs/rename", { id: plug.id, name: name }).then(function (body) {
        drafts.confirm(fieldId(plug.id), body.name != null ? body.name : name);
        refresh();
      });
    });
    const revokeBtn = document.createElement("button");
    revokeBtn.type = "button";
    revokeBtn.className = "ghost-btn";
    revokeBtn.textContent = "Revoke";
    revokeBtn.addEventListener("click", function () {
      drafts.revert(fieldId(plug.id));
      postJson("/api/plugs/revoke", { id: plug.id }).then(refresh);
    });
    const histBtn = document.createElement("button");
    histBtn.type = "button";
    histBtn.className = "ghost-btn";
    histBtn.textContent = "History";
    histBtn.addEventListener("click", function () {
      gotoHistory(plug.id);
    });
    actions.appendChild(renameBtn);
    actions.appendChild(revokeBtn);
    actions.appendChild(histBtn);
  }

  function paintPostingBubble(plug) {
    const bubble = ensureBubble(postingHost, plug.id);
    paintMetrics(bubble, plug, plug.id);
    const input = bindNameInput(bubble, plug.id, "");
    input.placeholder = "Name (optional)";
    const actions = bubble.querySelector(".plug-bubble-actions");
    if (actions.getAttribute("data-bound")) {
      return;
    }
    actions.setAttribute("data-bound", "1");
    const authBtn = document.createElement("button");
    authBtn.type = "button";
    authBtn.textContent = "Authorize";
    authBtn.addEventListener("click", function () {
      const name = input.value;
      postJson("/api/plugs/authorize", {
        id: plug.id,
        name: name || undefined,
      }).then(function (body) {
        if (body.name) {
          drafts.confirm(fieldId(plug.id), body.name);
        } else {
          drafts.revert(fieldId(plug.id));
        }
        refresh();
      });
    });
    actions.appendChild(authBtn);
  }

  function renderAuthorized(list) {
    const keep = (list || []).map(function (plug) { return plug.id; });
    (list || []).forEach(paintAuthorizedBubble);
    pruneHost(
      authorizedHost,
      keep,
      "None yet. Enable listen, then authorize a posting plug."
    );
  }

  function renderPosting(list) {
    const unknowns = (list || []).filter(function (plug) {
      return !plug.authorized;
    });
    const keep = unknowns.map(function (plug) { return plug.id; });
    unknowns.forEach(paintPostingBubble);
    const emptyText = listenBox && drafts.overlay(LISTEN_FIELD, !!listenBox.checked)
      ? "Waiting for UDP frames..."
      : "Turn on listen to discover unknown plugs.";
    pruneHost(postingHost, keep, emptyText);
  }

  function applyListen(serverListen) {
    if (!listenBox) {
      return;
    }
    if (document.activeElement !== listenBox) {
      listenBox.checked = !!drafts.overlay(LISTEN_FIELD, !!serverListen);
    }
  }

  function refresh() {
    if (global.WattcycleAuth && global.WattcycleAuth.guest) {
      return;
    }
    fetch("/api/plugs", { credentials: "same-origin", cache: "no-store" })
      .then(function (res) {
        if (!res.ok) {
          throw new Error("plugs unavailable");
        }
        return res.json();
      })
      .then(function (body) {
        applyListen(!!body.listen);
        renderAuthorized(body.authorized || []);
        renderPosting(body.posting || []);
      })
      .catch(function () {
        if (authorizedHost) {
          pruneHost(authorizedHost, [], "Plugs API unavailable on this hub.");
        }
      });
  }

  function bind() {
    listenBox = document.getElementById("plugsListen");
    authorizedHost = document.getElementById("plugsAuthorized");
    postingHost = document.getElementById("plugsPosting");
    if (!listenBox || !authorizedHost || !postingHost) {
      return;
    }
    if (!bound) {
      bound = true;
      listenBox.addEventListener("change", function () {
        drafts.beginEdit(LISTEN_FIELD, !!listenBox.checked);
        postJson("/api/plugs/listen", { open: !!listenBox.checked })
          .then(function (body) {
            drafts.confirm(LISTEN_FIELD, !!body.listen);
            refresh();
          })
          .catch(function () {
            drafts.revert(LISTEN_FIELD);
            refresh();
          });
      });
    }
    refresh();
    if (timer) {
      clearInterval(timer);
    }
    timer = setInterval(refresh, 3000);
  }

  global.WattcyclePlugs = { bind: bind, refresh: refresh };
  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.plugs = refresh;
})(window);
