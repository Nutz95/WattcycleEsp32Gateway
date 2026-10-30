(function (global) {
  const DB_NAME = "wattcycle-tariffs";
  const DB_VERSION = 1;
  const STORE = "config";
  const KEY = "tariffs";

  function defaultConfig() {
    return {
      currency: "EUR",
      windows: [
        {
          id: "default",
          from: "01-01",
          to: "12-31",
          bands: [
            {
              name: "HP",
              euroPerKwh: 0.2,
              intervals: [{ start: "06:00", end: "22:00" }],
            },
            {
              name: "HC",
              euroPerKwh: 0.12,
              intervals: [{ start: "22:00", end: "06:00" }],
            },
          ],
        },
      ],
    };
  }

  function openDb() {
    return new Promise(function (resolve, reject) {
      if (!global.indexedDB) {
        reject(new Error("indexedDB_unavailable"));
        return;
      }
      const req = global.indexedDB.open(DB_NAME, DB_VERSION);
      req.onupgradeneeded = function () {
        const db = req.result;
        if (!db.objectStoreNames.contains(STORE)) {
          db.createObjectStore(STORE);
        }
      };
      req.onsuccess = function () {
        resolve(req.result);
      };
      req.onerror = function () {
        reject(req.error || new Error("idb_open_failed"));
      };
    });
  }

  async function load() {
    try {
      const db = await openDb();
      const value = await new Promise(function (resolve, reject) {
        const tx = db.transaction(STORE, "readonly");
        const req = tx.objectStore(STORE).get(KEY);
        req.onsuccess = function () {
          resolve(req.result || null);
        };
        req.onerror = function () {
          reject(req.error);
        };
      });
      if (!value || !Array.isArray(value.windows)) {
        return defaultConfig();
      }
      return {
        currency: value.currency || "EUR",
        windows: value.windows,
      };
    } catch (error) {
      return defaultConfig();
    }
  }

  async function save(config) {
    const db = await openDb();
    await new Promise(function (resolve, reject) {
      const tx = db.transaction(STORE, "readwrite");
      tx.objectStore(STORE).put(config, KEY);
      tx.oncomplete = function () {
        resolve();
      };
      tx.onerror = function () {
        reject(tx.error);
      };
    });
  }

  function exportJson(config) {
    return JSON.stringify(config, null, 2);
  }

  function importJson(text) {
    const parsed = JSON.parse(text);
    if (!parsed || typeof parsed !== "object" || !Array.isArray(parsed.windows)) {
      throw new Error("JSON invalide : champ windows manquant.");
    }
    return global.WattcycleTariffValidate.validateConfig({
      currency: typeof parsed.currency === "string" ? parsed.currency : "EUR",
      windows: parsed.windows,
    });
  }

  async function saveValidated(config) {
    const clean = global.WattcycleTariffValidate.validateConfig(config);
    await save(clean);
    return clean;
  }

  global.WattcycleTariffStore = {
    defaultConfig: defaultConfig,
    load: load,
    save: save,
    saveValidated: saveValidated,
    exportJson: exportJson,
    importJson: importJson,
  };
})(window);
