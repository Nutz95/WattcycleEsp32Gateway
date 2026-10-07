(function (global) {
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

  function bind() {
    const button = document.getElementById("logoutBtn");
    if (!button || button.dataset.bound === "1") {
      return;
    }
    button.dataset.bound = "1";
    button.addEventListener("click", logout);
  }

  global.WattcycleViews = global.WattcycleViews || {};
  global.WattcycleViews.account = function () {
    bind();
  };
})(window);
