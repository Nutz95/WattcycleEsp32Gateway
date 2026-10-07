/* Server-gated auth UI (no client-side authorization decisions). */
(function (global) {
  const authRoot = document.getElementById("authRoot");
  const appShell = document.getElementById("appShell");

  function showAuth() {
    if (authRoot) authRoot.hidden = false;
    if (appShell) appShell.hidden = true;
  }

  function showApp() {
    if (authRoot) authRoot.hidden = true;
    if (appShell) appShell.hidden = false;
  }

  function setAuthHtml(html) {
    if (authRoot) authRoot.innerHTML = html;
  }

  async function fetchStatus() {
    const response = await fetch("/api/auth/status", { cache: "no-store", credentials: "same-origin" });
    if (!response.ok) {
      throw new Error("status " + response.status);
    }
    return response.json();
  }

  async function postJson(path, body) {
    const response = await fetch(path, {
      method: "POST",
      credentials: "same-origin",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body)
    });
    let payload = {};
    try {
      payload = await response.json();
    } catch (ignored) {}
    return { response: response, payload: payload };
  }

  function renderLogin(message) {
    setAuthHtml(
      '<section class="auth-shell">' +
        "<h1>Sign in</h1>" +
        '<p class="hint">Credentials are verified on the gateway. Telemetry stays locked until login succeeds.</p>' +
        '<p class="error" id="loginError" hidden></p>' +
        '<label for="loginUser">Username</label>' +
        '<input id="loginUser" autocomplete="username" maxlength="32" spellcheck="false" />' +
        '<label for="loginPass">Password</label>' +
        '<input id="loginPass" type="password" autocomplete="current-password" maxlength="64" />' +
        '<button type="button" id="loginBtn">Sign in</button>' +
        '<p class="hint">Forgot password? Hold the bottom button (GPIO0) for 3s on the device, then confirm with the top button.</p>' +
      "</section>"
    );
    if (message) {
      const errorEl = document.getElementById("loginError");
      errorEl.hidden = false;
      errorEl.textContent = message;
    }
    document.getElementById("loginBtn").addEventListener("click", async function () {
      const username = document.getElementById("loginUser").value.trim();
      const password = document.getElementById("loginPass").value;
      const result = await postJson("/api/auth/login", { username: username, password: password });
      if (!result.response.ok) {
        renderLogin(result.payload.error || "Login failed");
        return;
      }
      global.WattcycleAuth.onAuthenticated();
    });
  }

  function renderSetup() {
    setAuthHtml(
      '<section class="auth-shell">' +
        "<h1>Create gateway login</h1>" +
        '<p class="hint">First boot: username ≤32 ([A-Za-z0-9._-]), password 8–64 printable ASCII. Confirm on the device with the <strong>top</strong> button.</p>' +
        '<label for="setupUser">Username</label>' +
        '<input id="setupUser" autocomplete="username" maxlength="32" spellcheck="false" pattern="[A-Za-z0-9._\\-]{1,32}" />' +
        '<label for="setupPass">Password</label>' +
        '<input id="setupPass" type="password" autocomplete="new-password" maxlength="64" minlength="8" />' +
        '<button type="button" id="setupBtn">Continue</button>' +
        '<p class="error" id="setupError" hidden></p>' +
      "</section>"
    );
    document.getElementById("setupBtn").addEventListener("click", async function () {
      const username = document.getElementById("setupUser").value.trim();
      const password = document.getElementById("setupPass").value;
      const errorEl = document.getElementById("setupError");
      const result = await postJson("/api/auth/setup", { username: username, password: password });
      if (!result.response.ok) {
        errorEl.hidden = false;
        errorEl.textContent = result.payload.error || "Setup rejected";
        return;
      }
      renderWaitingConfirm();
    });
  }

  function renderWaitingConfirm() {
    setAuthHtml(
      '<section class="auth-shell">' +
        "<h1>Confirm on device</h1>" +
        '<p class="hint">On the TTGO screen: press <strong>TOP</strong> to validate, <strong>BOTTOM</strong> to cancel.</p>' +
        '<p class="hint" id="waitStatus">Waiting for physical confirmation…</p>' +
      "</section>"
    );
    const timer = setInterval(async function () {
      try {
        const status = await fetchStatus();
        if (status.configured && !status.pendingSetup) {
          clearInterval(timer);
          renderLogin("Credentials saved. Sign in.");
        } else if (!status.pendingSetup && !status.configured) {
          clearInterval(timer);
          renderSetup();
        }
      } catch (error) {
        document.getElementById("waitStatus").textContent = "Waiting… (" + error.message + ")";
      }
    }, 1000);
  }

  async function bootstrap(onAuthenticated) {
    global.WattcycleAuth.onAuthenticated = onAuthenticated;
    showAuth();
    try {
      const status = await fetchStatus();
      if (status.authenticated) {
        showApp();
        onAuthenticated();
        return;
      }
      if (!status.configured) {
        if (status.pendingSetup) {
          renderWaitingConfirm();
        } else {
          renderSetup();
        }
        return;
      }
      renderLogin(status.pendingReset ? "Password reset pending on device." : "");
    } catch (error) {
      renderLogin("Cannot reach auth API: " + error.message);
    }
  }

  function handleUnauthorized() {
    showAuth();
    renderLogin("Session expired. Sign in again.");
  }

  global.WattcycleAuth = {
    bootstrap: bootstrap,
    handleUnauthorized: handleUnauthorized,
    showApp: showApp
  };
})(window);
