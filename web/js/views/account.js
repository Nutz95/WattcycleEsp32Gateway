(function (global) {
  const guestTabNames = [
    "overview", "dashboard", "solar", "wattcycle", "ecoflow", "history",
    "cells", "temperatures", "warnings", "gateway", "esp"
  ];

  async function logout() {
    const errorEl = document.getElementById("logoutError");
    if (errorEl) {
      errorEl.hidden = true;
      errorEl.textContent = "";
    }
    try {
      const response = await fetch("/api/auth/logout", {
        method: "POST",
        credentials: "same-origin",
        cache: "no-store"
      });
      if (!response.ok) {
        throw new Error("HTTP " + response.status);
      }
      global.WattcycleAuth.handleUnauthorized();
    } catch (error) {
      if (errorEl) {
        errorEl.hidden = false;
        errorEl.textContent = "Sign out failed: " + error.message;
      }
    }
  }

  function tabMask() {
    let mask = 0;
    guestTabNames.forEach(function (name, index) {
      const box = document.querySelector('#guestTabs input[data-tab="' + name + '"]');
      if (box && box.checked) {
        mask |= 1 << index;
      }
    });
    return mask;
  }

  function showTabMask(mask) {
    guestTabNames.forEach(function (name, index) {
      const box = document.querySelector('#guestTabs input[data-tab="' + name + '"]');
      if (box) {
        box.checked = (mask & (1 << index)) !== 0;
      }
    });
  }

  function ensureTabBoxes() {
    const host = document.getElementById("guestTabs");
    if (!host || host.dataset.built === "1") {
      return;
    }
    host.dataset.built = "1";
    guestTabNames.forEach(function (name) {
      const label = document.createElement("label");
      label.className = "hint";
      const input = document.createElement("input");
      input.type = "checkbox";
      input.dataset.tab = name;
      label.appendChild(input);
      label.appendChild(document.createTextNode(" " + name));
      host.appendChild(label);
      input.addEventListener("change", saveGuest);
    });
  }

  async function saveGuest() {
    const box = document.getElementById("guestAccess");
    const errorEl = document.getElementById("guestError");
    if (!box || box.disabled) {
      return;
    }
    if (errorEl) {
      errorEl.hidden = true;
    }
    const response = await fetch("/api/guest", {
      method: "POST",
      credentials: "same-origin",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ enabled: box.checked, tabs: tabMask() })
    });
    if (!response.ok && errorEl) {
      errorEl.hidden = false;
      errorEl.textContent = "Guest switch was not saved (" + response.status + ").";
    }
    if (global.WattcycleGuest) {
      global.WattcycleGuest.refresh();
    }
  }

  async function loadGuest() {
    const box = document.getElementById("guestAccess");
    const signIn = document.getElementById("signInBtn");
    const logoutBtn = document.getElementById("logoutBtn");
    const errorEl = document.getElementById("guestError");
    if (!box) {
      return;
    }
    ensureTabBoxes();
    let authenticated = false;
    try {
      const status = await fetch("/api/auth/status", { cache: "no-store", credentials: "same-origin" });
      const auth = status.ok ? await status.json() : {};
      authenticated = !!auth.authenticated;
      const guest = await fetch("/api/guest", { cache: "no-store", credentials: "same-origin" });
      if (guest.ok) {
        const payload = await guest.json();
        box.checked = !!payload.enabled;
        showTabMask(typeof payload.tabs === "number" ? payload.tabs : (1 << guestTabNames.length) - 1);
      }
    } catch (error) {
      if (errorEl) {
        errorEl.hidden = false;
        errorEl.textContent = "Guest status unavailable.";
      }
    }
    box.disabled = !authenticated;
    document.querySelectorAll("#guestTabs input").forEach(function (input) {
      input.disabled = !authenticated;
    });
    if (signIn) {
      signIn.hidden = authenticated;
    }
    if (logoutBtn) {
      logoutBtn.hidden = !authenticated;
    }
  }

  function bind() {
    const button = document.getElementById("logoutBtn");
    if (button && button.dataset.bound !== "1") {
      button.dataset.bound = "1";
      button.addEventListener("click", logout);
    }
    const signIn = document.getElementById("signInBtn");
    if (signIn && signIn.dataset.bound !== "1") {
      signIn.dataset.bound = "1";
      signIn.addEventListener("click", function () {
        if (global.WattcycleAuth && global.WattcycleAuth.requestSignIn) {
          global.WattcycleAuth.requestSignIn();
        }
      });
    }
    const box = document.getElementById("guestAccess");
    if (box && box.dataset.bound !== "1") {
      box.dataset.bound = "1";
      box.addEventListener("change", saveGuest);
    }
    loadGuest();
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.account = function () {
    bind();
  };
})(window);
