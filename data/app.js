async function refresh() {
  try {
    const response = await fetch("/api/telemetry", { cache: "no-store" });
    if (!response.ok) {
      throw new Error("HTTP " + response.status);
    }
    const data = await response.json();
    render(data);
  } catch (error) {
    document.getElementById("summary").textContent = "API unreachable: " + error.message;
  }
}

function render(data) {
  const soc = document.getElementById("soc");
  soc.textContent = data.valid ? data.soc + "%" : "--%";

  document.getElementById("summary").textContent = data.valid
    ? "Live telemetry from Wattcycle BMS"
    : "No valid BMS snapshot yet";

  document.getElementById("voltage").textContent = fmt(data.voltage, "V", 2);
  document.getElementById("current").textContent = fmt(data.current, "A", 2);
  document.getElementById("power").textContent = fmt(data.power, "W", 0);
  document.getElementById("capacity").textContent =
    fmt(data.remainingAh, "", 1) + " / " + fmt(data.totalAh, "Ah", 1);
  document.getElementById("cycles").textContent = data.cycles ?? "--";
  document.getElementById("temps").textContent =
    "MOS " + fmt(data.mosTemp, "°", 1) + " / PCB " + fmt(data.pcbTemp, "°", 1);

  const cells = document.getElementById("cells");
  cells.innerHTML = "";
  (data.cells || []).forEach((voltage, index) => {
    const div = document.createElement("div");
    div.className = "cell";
    div.innerHTML = "<small>C" + (index + 1) + "</small>" + voltage.toFixed(3) + " V";
    cells.appendChild(div);
  });

  setLink("wifi", data.gateway && data.gateway.wifi, data.gateway && data.gateway.ip);
  setLink("ble", data.gateway && data.gateway.ble, data.gateway && data.gateway.ble ? "connected" : "down");
  document.getElementById("bleAddress").textContent =
    (data.gateway && data.gateway.bleAddress) || "--";
  document.getElementById("error").textContent =
    (data.gateway && data.gateway.error) || "none";
}

function setLink(id, ok, label) {
  const el = document.getElementById(id);
  el.textContent = label || (ok ? "ok" : "down");
  el.className = ok ? "ok" : "bad";
}

function fmt(value, suffix, digits) {
  if (value === undefined || value === null || Number.isNaN(value)) {
    return "--" + (suffix ? " " + suffix : "");
  }
  return Number(value).toFixed(digits) + (suffix ? " " + suffix : "");
}

refresh();
setInterval(refresh, 1000);
