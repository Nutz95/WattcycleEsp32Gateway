(function (global) {
  function createShellyDeviceAdmin(options) {
    const drafts = new global.WattcycleDraftStore();
    const listenField = options.listenField;
    const nameFieldPrefix = options.nameFieldPrefix;
    const dataAttr = options.dataAttr;
    const apiBase = options.apiBase;
    const bubbleClass = options.bubbleClass || "plug-bubble";
    const paintMetrics = options.paintMetrics;
    const emptyAuthorized = options.emptyAuthorized;
    const emptyPostingOpen = options.emptyPostingOpen;
    const emptyPostingClosed = options.emptyPostingClosed;
    const afterRefresh = options.afterRefresh || function () {};
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

    function fieldId(deviceId) {
      return nameFieldPrefix + deviceId;
    }

    function ensureBubble(host, deviceId) {
      let bubble = host.querySelector("[" + dataAttr + '="' + deviceId + '"]');
      if (bubble) {
        return bubble;
      }
      bubble = document.createElement("article");
      bubble.className = bubbleClass;
      bubble.setAttribute(dataAttr, deviceId);
      bubble.innerHTML =
        '<div class="plug-bubble-head">' +
        '<p class="plug-bubble-title"></p>' +
        '<p class="plug-bubble-id hint"></p>' +
        "</div>" +
        '<p class="plug-bubble-power"></p>' +
        '<div class="plug-bubble-metrics"></div>' +
        '<div class="plug-bubble-edit"></div>' +
        '<div class="plug-bubble-actions"></div>';
      host.appendChild(bubble);
      return bubble;
    }

    function pruneHost(host, keepIds, emptyText) {
      Array.prototype.slice.call(host.children).forEach(function (child) {
        const id = child.getAttribute(dataAttr);
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

    function bindNameInput(bubble, deviceId, serverName) {
      const edit = bubble.querySelector(".plug-bubble-edit");
      let input = edit.querySelector("input");
      if (!input) {
        input = document.createElement("input");
        input.type = "text";
        input.maxLength = 24;
        input.placeholder = "Display name";
        input.addEventListener("input", function () {
          drafts.beginEdit(fieldId(deviceId), input.value);
        });
        edit.appendChild(input);
      }
      if (document.activeElement !== input) {
        input.value = drafts.overlay(fieldId(deviceId), serverName || "");
      }
      return input;
    }

    function paintAuthorizedBubble(device) {
      const bubble = ensureBubble(authorizedHost, device.id);
      paintMetrics(bubble, device, device.name || device.id);
      const input = bindNameInput(bubble, device.id, device.name || "");
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
        postJson(apiBase + "/rename", { id: device.id, name: name }).then(function (body) {
          drafts.confirm(fieldId(device.id), body.name != null ? body.name : name);
          refresh();
        });
      });
      const revokeBtn = document.createElement("button");
      revokeBtn.type = "button";
      revokeBtn.className = "ghost-btn";
      revokeBtn.textContent = "Revoke";
      revokeBtn.addEventListener("click", function () {
        drafts.revert(fieldId(device.id));
        postJson(apiBase + "/revoke", { id: device.id }).then(refresh);
      });
      actions.appendChild(renameBtn);
      actions.appendChild(revokeBtn);
      if (options.onAuthorizedActions) {
        options.onAuthorizedActions(actions, device);
      }
    }

    function paintPostingBubble(device) {
      const bubble = ensureBubble(postingHost, device.id);
      paintMetrics(bubble, device, device.id);
      const input = bindNameInput(bubble, device.id, "");
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
        postJson(apiBase + "/authorize", {
          id: device.id,
          name: name || undefined,
        }).then(function (body) {
          if (body.name) {
            drafts.confirm(fieldId(device.id), body.name);
          } else {
            drafts.revert(fieldId(device.id));
          }
          refresh();
        });
      });
      actions.appendChild(authBtn);
    }

    function applyListen(serverListen) {
      if (!listenBox) {
        return;
      }
      if (document.activeElement !== listenBox) {
        listenBox.checked = !!drafts.overlay(listenField, !!serverListen);
      }
    }

    function refresh() {
      if (global.WattcycleAuth && global.WattcycleAuth.guest) {
        return;
      }
      fetch(apiBase, { credentials: "same-origin", cache: "no-store" })
        .then(function (res) {
          if (!res.ok) {
            throw new Error("unavailable");
          }
          return res.json();
        })
        .then(function (body) {
          applyListen(!!body.listen);
          const authorized = body.authorized || [];
          if (authorizedHost) {
            const keep = authorized.map(function (device) {
              return device.id;
            });
            authorized.forEach(paintAuthorizedBubble);
            pruneHost(authorizedHost, keep, emptyAuthorized);
            const unknowns = (body.posting || []).filter(function (device) {
              return !device.authorized;
            });
            const postKeep = unknowns.map(function (device) {
              return device.id;
            });
            unknowns.forEach(paintPostingBubble);
            const emptyText =
              listenBox && drafts.overlay(listenField, !!listenBox.checked)
                ? emptyPostingOpen
                : emptyPostingClosed;
            pruneHost(postingHost, postKeep, emptyText);
          }
          afterRefresh(body);
        })
        .catch(function () {
          if (authorizedHost) {
            pruneHost(authorizedHost, [], options.unavailableText || "API unavailable.");
          }
          afterRefresh(null);
        });
    }

    function bind(ids) {
      listenBox = document.getElementById(ids.listen);
      authorizedHost = document.getElementById(ids.authorized);
      postingHost = document.getElementById(ids.posting);
      if (!listenBox || !authorizedHost || !postingHost) {
        return;
      }
      if (!bound) {
        bound = true;
        listenBox.addEventListener("change", function () {
          drafts.beginEdit(listenField, !!listenBox.checked);
          postJson(apiBase + "/listen", { open: !!listenBox.checked })
            .then(function (body) {
              drafts.confirm(listenField, !!body.listen);
              refresh();
            })
            .catch(function () {
              drafts.revert(listenField);
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

    return { bind: bind, refresh: refresh };
  }

  global.WattcycleShellyDeviceAdmin = { create: createShellyDeviceAdmin };
})(window);
