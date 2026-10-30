(function (global) {
  let root = null;
  let draft = null;
  let onChange = null;

  function cloneConfig(config) {
    return JSON.parse(JSON.stringify(config));
  }

  function ensureDraft() {
    if (!draft) {
      draft = cloneConfig(global.WattcycleTariffStore.defaultConfig());
    }
    return draft;
  }

  function emit() {
    if (onChange) onChange(cloneConfig(ensureDraft()));
  }

  function el(tag, className, text) {
    const node = document.createElement(tag);
    if (className) node.className = className;
    if (text != null) node.textContent = text;
    return node;
  }

  function labeledInput(labelText, input) {
    const label = el("label", "tariff-field");
    label.appendChild(document.createTextNode(labelText));
    label.appendChild(input);
    return label;
  }

  function numberInput(value, min, max, step) {
    const input = document.createElement("input");
    input.type = "number";
    input.min = String(min);
    input.max = String(max);
    input.step = String(step);
    input.value = String(value);
    return input;
  }

  function textInput(value, maxLength, placeholder) {
    const input = document.createElement("input");
    input.type = "text";
    input.maxLength = maxLength;
    input.value = value || "";
    if (placeholder) input.placeholder = placeholder;
    return input;
  }

  function renderInterval(band, interval, iIndex, wIndex, bIndex) {
    const row = el("div", "tariff-interval-row");
    const start = textInput(interval.start, 5, "22:00");
    const end = textInput(interval.end, 5, "06:00");
    start.addEventListener("change", function () {
      draft.windows[wIndex].bands[bIndex].intervals[iIndex].start = start.value.trim();
      emit();
    });
    end.addEventListener("change", function () {
      draft.windows[wIndex].bands[bIndex].intervals[iIndex].end = end.value.trim();
      emit();
    });
    row.appendChild(labeledInput("Début", start));
    row.appendChild(labeledInput("Fin", end));
    const remove = el("button", "ghost-btn", "Retirer créneau");
    remove.type = "button";
    remove.addEventListener("click", function () {
      draft.windows[wIndex].bands[bIndex].intervals.splice(iIndex, 1);
      paint();
      emit();
    });
    row.appendChild(remove);
    return row;
  }

  function renderBand(band, bIndex, wIndex) {
    const box = el("div", "tariff-band");
    box.appendChild(el("h4", null, "Bande " + (bIndex + 1)));
    const name = textInput(band.name, 16, "HP");
    name.addEventListener("change", function () {
      draft.windows[wIndex].bands[bIndex].name = name.value.trim();
      emit();
    });
    const rate = numberInput(band.euroPerKwh, 0, 5, 0.0001);
    rate.addEventListener("change", function () {
      draft.windows[wIndex].bands[bIndex].euroPerKwh = Number(rate.value);
      emit();
    });
    box.appendChild(labeledInput("Nom", name));
    box.appendChild(labeledInput("€ / kWh", rate));
    (band.intervals || []).forEach(function (interval, iIndex) {
      box.appendChild(renderInterval(band, interval, iIndex, wIndex, bIndex));
    });
    const addInterval = el("button", "ghost-btn", "Ajouter créneau");
    addInterval.type = "button";
    addInterval.addEventListener("click", function () {
      draft.windows[wIndex].bands[bIndex].intervals.push({ start: "00:00", end: "06:00" });
      paint();
      emit();
    });
    const removeBand = el("button", "ghost-btn", "Supprimer bande");
    removeBand.type = "button";
    removeBand.addEventListener("click", function () {
      draft.windows[wIndex].bands.splice(bIndex, 1);
      paint();
      emit();
    });
    box.appendChild(addInterval);
    box.appendChild(removeBand);
    return box;
  }

  function renderWindow(win, wIndex) {
    const box = el("div", "tariff-window");
    box.appendChild(el("h3", null, "Période " + (wIndex + 1)));
    const from = textInput(win.from, 5, "01-01");
    const to = textInput(win.to, 5, "12-31");
    from.addEventListener("change", function () {
      draft.windows[wIndex].from = from.value.trim();
      emit();
    });
    to.addEventListener("change", function () {
      draft.windows[wIndex].to = to.value.trim();
      emit();
    });
    box.appendChild(labeledInput("Du (MM-JJ)", from));
    box.appendChild(labeledInput("Au (MM-JJ)", to));
    box.appendChild(el("p", "hint", "Exemple : 01-01 → 12-31 pour toute l’année (UTC locale du navigateur)."));
    (win.bands || []).forEach(function (band, bIndex) {
      box.appendChild(renderBand(band, bIndex, wIndex));
    });
    const addBand = el("button", "ghost-btn", "Ajouter bande");
    addBand.type = "button";
    addBand.addEventListener("click", function () {
      draft.windows[wIndex].bands.push({
        name: "HP",
        euroPerKwh: 0.2,
        intervals: [{ start: "06:00", end: "22:00" }],
      });
      paint();
      emit();
    });
    const removeWin = el("button", "ghost-btn", "Supprimer période");
    removeWin.type = "button";
    removeWin.addEventListener("click", function () {
      draft.windows.splice(wIndex, 1);
      paint();
      emit();
    });
    box.appendChild(addBand);
    box.appendChild(removeWin);
    return box;
  }

  function paint() {
    if (!root) return;
    root.innerHTML = "";
    const cfg = ensureDraft();
    const currency = textInput(cfg.currency || "EUR", 3, "EUR");
    currency.addEventListener("change", function () {
      draft.currency = currency.value.trim().toUpperCase();
      emit();
    });
    root.appendChild(labeledInput("Devise (ISO)", currency));
    (cfg.windows || []).forEach(function (win, wIndex) {
      root.appendChild(renderWindow(win, wIndex));
    });
    const addWin = el("button", "ghost-btn", "Ajouter période");
    addWin.type = "button";
    addWin.addEventListener("click", function () {
      draft.windows.push({
        id: "w" + Date.now(),
        from: "01-01",
        to: "12-31",
        bands: [
          { name: "HP", euroPerKwh: 0.2, intervals: [{ start: "06:00", end: "22:00" }] },
          { name: "HC", euroPerKwh: 0.12, intervals: [{ start: "22:00", end: "06:00" }] },
        ],
      });
      paint();
      emit();
    });
    root.appendChild(addWin);
  }

  function mount(container, config, changeHandler) {
    root = container;
    draft = cloneConfig(config || global.WattcycleTariffStore.defaultConfig());
    onChange = changeHandler;
    paint();
  }

  function getDraft() {
    return cloneConfig(ensureDraft());
  }

  function setDraft(config) {
    draft = cloneConfig(config);
    paint();
    emit();
  }

  global.WattcycleTariffEditor = {
    mount: mount,
    getDraft: getDraft,
    setDraft: setDraft,
  };
})(window);
