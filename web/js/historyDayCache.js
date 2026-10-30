(function (global) {
  const DB_NAME = "wattcycle-history-days";
  const DB_VERSION = 3;
  const STORE = "days";

  function utcToday() {
    const d = new Date();
    const y = d.getUTCFullYear();
    const m = String(d.getUTCMonth() + 1).padStart(2, "0");
    const day = String(d.getUTCDate()).padStart(2, "0");
    return y + "-" + m + "-" + day;
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
        if (db.objectStoreNames.contains(STORE)) {
          db.deleteObjectStore(STORE);
        }
        db.createObjectStore(STORE, { keyPath: "key" });
      };
      req.onsuccess = function () {
        resolve(req.result);
      };
      req.onerror = function () {
        reject(req.error || new Error("idb_open_failed"));
      };
    });
  }

  function dayKey(kind, date) {
    return "v3|" + kind + "|" + date;
  }

  async function getRecord(kind, date) {
    try {
      const db = await openDb();
      return await new Promise(function (resolve, reject) {
        const tx = db.transaction(STORE, "readonly");
        const req = tx.objectStore(STORE).get(dayKey(kind, date));
        req.onsuccess = function () {
          resolve(req.result || null);
        };
        req.onerror = function () {
          reject(req.error);
        };
      });
    } catch (error) {
      return null;
    }
  }

  async function putRecord(kind, date, etag, gzipBytes) {
    try {
      const db = await openDb();
      await new Promise(function (resolve, reject) {
        const tx = db.transaction(STORE, "readwrite");
        tx.objectStore(STORE).put({
          key: dayKey(kind, date),
          kind: kind,
          date: date,
          etag: etag || "",
          gzip: gzipBytes,
          savedAt: Date.now(),
        });
        tx.oncomplete = function () {
          resolve();
        };
        tx.onerror = function () {
          reject(tx.error);
        };
      });
    } catch (error) {
      // Cache is best-effort.
    }
  }

  async function deleteRecord(kind, date) {
    try {
      const db = await openDb();
      await new Promise(function (resolve, reject) {
        const tx = db.transaction(STORE, "readwrite");
        tx.objectStore(STORE).delete(dayKey(kind, date));
        tx.oncomplete = function () {
          resolve();
        };
        tx.onerror = function () {
          reject(tx.error);
        };
      });
    } catch (error) {
      // ignore
    }
  }

  function canGunzip() {
    return typeof global.DecompressionStream === "function";
  }

  async function gunzipToText(bytes) {
    if (!canGunzip()) {
      throw new Error("DecompressionStream_unavailable");
    }
    const input = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
    if (input.byteLength < 18 || input[0] !== 0x1f || input[1] !== 0x8b) {
      throw new Error("not_gzip");
    }
    const stream = new Response(input).body.pipeThrough(new DecompressionStream("gzip"));
    return await new Response(stream).text();
  }

  async function readBodyBytes(response, onProgress) {
    const total = Number(response.headers.get("Content-Length") || 0);
    if (!response.body || !response.body.getReader) {
      return new Uint8Array(await response.arrayBuffer());
    }
    const reader = response.body.getReader();
    const chunks = [];
    let received = 0;
    for (;;) {
      const step = await reader.read();
      if (step.done) break;
      chunks.push(step.value);
      received += step.value.byteLength;
      if (onProgress) onProgress(received, total);
    }
    const merged = new Uint8Array(received);
    let offset = 0;
    chunks.forEach(function (chunk) {
      merged.set(chunk, offset);
      offset += chunk.byteLength;
    });
    return merged;
  }

  async function fetchIdentityCsv(path, date, onProgress) {
    const url = path + "?date=" + encodeURIComponent(date) + "&encoding=identity";
    const response = await fetch(url, {
      credentials: "same-origin",
      cache: "no-store",
    });
    if (!response.ok) {
      const err = new Error("day_http_" + response.status);
      err.status = response.status;
      throw err;
    }
    const bytes = await readBodyBytes(response, onProgress);
    return new TextDecoder().decode(bytes);
  }

  async function fetchGzipCsv(kind, date, path, options, cached) {
    const opts = options || {};
    const url = path + "?date=" + encodeURIComponent(date) + "&encoding=gzip";
    const headers = {};
    if (cached && cached.etag) {
      headers["If-None-Match"] = cached.etag;
    }
    const response = await fetch(url, {
      credentials: "same-origin",
      cache: "no-store",
      headers: headers,
    });
    if (response.status === 304 && cached && cached.gzip) {
      return gunzipToText(cached.gzip);
    }
    if (!response.ok) {
      const err = new Error("day_http_" + response.status);
      err.status = response.status;
      throw err;
    }
    const contentType = String(response.headers.get("Content-Type") || "");
    const bytes = await readBodyBytes(response, opts.onProgress);
    // Server may fall back to plain CSV when gzip verify fails (single response).
    if (contentType.indexOf("text/csv") >= 0 || bytes[0] !== 0x1f) {
      return new TextDecoder().decode(bytes);
    }
    const text = await gunzipToText(bytes);
    const etag = response.headers.get("ETag") || "";
    await putRecord(kind, date, etag, bytes);
    return text;
  }

  /**
   * Fetch a day CSV via gzip (+ IndexedDB). Identity only when DecompressionStream is missing.
   * @param {"pack"|"plugs"} kind
   * @param {string} date YYYY-MM-DD
   * @param {string} path API path without query
   * @param {{onProgress?: function(number,number)}|undefined} options
   */
  async function fetchDayCsv(kind, date, path, options) {
    const opts = options || {};
    if (!canGunzip()) {
      return fetchIdentityCsv(path, date, opts.onProgress);
    }
    const isToday = date === utcToday();
    const cached = await getRecord(kind, date);
    if (!isToday && cached && cached.gzip) {
      try {
        return await gunzipToText(cached.gzip);
      } catch (error) {
        await deleteRecord(kind, date);
      }
    }
    return fetchGzipCsv(kind, date, path, opts, cached);
  }

  global.WattcycleHistoryDayCache = {
    fetchDayCsv: fetchDayCsv,
    utcToday: utcToday,
    getRecord: getRecord,
  };
})(window);
