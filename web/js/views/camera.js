(function (global) {
  let audioControl = null;
  let audioContext = null;
  let audioGain = null;
  let playAt = 0;
  let nextGainPoll = 0;
  let gainHoldUntil = 0;

  function applyPercent(percent) {
    if (audioGain) {
      audioGain.gain.value = percent / 16;
    }
  }

  async function postGain(percent) {
    await fetch("/api/mic-gain", {
      method: "POST",
      credentials: "same-origin",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ percent: percent })
    });
  }

  async function refreshGain() {
    const slider = document.getElementById("cameraVolume");
    if (Date.now() < gainHoldUntil || document.activeElement === slider) {
      return;
    }
    const response = await fetch("/api/mic-gain", { credentials: "same-origin", cache: "no-store" });
    if (!response.ok) {
      return;
    }
    const payload = await response.json();
    const percent = Number(payload.percent);
    if (!Number.isFinite(percent)) {
      return;
    }
    if (slider) {
      slider.value = String(percent);
    }
    applyPercent(percent);
  }

  function setMeter(peak) {
    const meter = document.getElementById("cameraMeter");
    if (!meter) {
      return;
    }
    const width = Math.min(100, Math.round((peak * 100) / 32767));
    meter.style.width = width + "%";
  }

  function streamUrl(port, path) {
    return location.protocol + "//" + location.hostname + ":" + port + path;
  }

  function stopAudio() {
    if (audioControl) {
      audioControl.abort();
      audioControl = null;
    }
    if (audioContext) {
      audioContext.close();
      audioContext = null;
    }
    audioGain = null;
    playAt = 0;
    setMeter(0);
  }

  const adpcmStep = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272,
    2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
  ];
  const adpcmIndex = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8];

  function decodeAdpcmFrame(frame) {
    let index = frame[1];
    if (index > 88) {
      index = 88;
    }
    let predictor = frame[2] | (frame[3] << 8);
    if (predictor >= 32768) {
      predictor -= 65536;
    }
    const samples = new Int16Array(640);
    for (let sample = 0; sample < 320; sample += 1) {
      const packed = frame[4 + (sample >> 1)];
      const nibble = (sample & 1) ? (packed >> 4) & 15 : packed & 15;
      const step = adpcmStep[index];
      let delta = step >> 3;
      if (nibble & 4) {
        delta += step;
      }
      if (nibble & 2) {
        delta += step >> 1;
      }
      if (nibble & 1) {
        delta += step >> 2;
      }
      predictor += nibble & 8 ? -delta : delta;
      if (predictor > 32767) {
        predictor = 32767;
      }
      if (predictor < -32768) {
        predictor = -32768;
      }
      index += adpcmIndex[nibble];
      if (index < 0) {
        index = 0;
      }
      if (index > 88) {
        index = 88;
      }
      samples[sample * 2] = predictor;
      samples[sample * 2 + 1] = predictor;
    }
    return samples;
  }

  function playPcm(context, samples) {
    const frames = Math.floor(samples.length / 2);
    if (frames < 1) {
      return;
    }
    const buffer = context.createBuffer(2, frames, 16000);
    const left = buffer.getChannelData(0);
    const right = buffer.getChannelData(1);
    let peak = 0;
    for (let index = 0; index < frames; index += 1) {
      const sampleL = samples[index * 2];
      const sampleR = samples[index * 2 + 1];
      left[index] = sampleL / 32768;
      right[index] = sampleR / 32768;
      peak = Math.max(peak, Math.abs(sampleL), Math.abs(sampleR));
    }
    setMeter(peak);
    const source = context.createBufferSource();
    source.buffer = buffer;
    source.connect(audioGain || context.destination);
    if (playAt < context.currentTime) {
      playAt = context.currentTime + 0.02;
    }
    source.start(playAt);
    playAt += buffer.duration;
  }

  async function pullAudio(signal, note, context) {
    const response = await fetch(streamUrl(47691, "/mic.adpcm"), {
      credentials: "include",
      cache: "no-store",
      signal: signal
    });
    if (!response.ok || !response.body) {
      note.textContent = "Microphone unavailable.";
      if (audioContext) {
        audioContext.close();
        audioContext = null;
      }
      return;
    }
    note.textContent = "";
    const reader = response.body.getReader();
    let pending = new Uint8Array(0);
    while (!signal.aborted) {
      const chunk = await reader.read();
      if (chunk.done) {
        break;
      }
      const merged = new Uint8Array(pending.length + chunk.value.length);
      merged.set(pending);
      merged.set(chunk.value, pending.length);
      let offset = 0;
      while (offset + 164 <= merged.length) {
        if (merged[offset] !== 0xAD) {
          offset += 1;
          continue;
        }
        playPcm(context, decodeAdpcmFrame(merged.subarray(offset, offset + 164)));
        offset += 164;
      }
      pending = merged.slice(offset);
      if (Date.now() >= nextGainPoll) {
        nextGainPoll = Date.now() + 2000;
        refreshGain();
      }
    }
  }

  function startAudio(note) {
    stopAudio();
    audioContext = new AudioContext({ sampleRate: 16000 });
    audioGain = audioContext.createGain();
    const slider = document.getElementById("cameraVolume");
    applyPercent(slider ? Number(slider.value) : 80);
    audioGain.connect(audioContext.destination);
    refreshGain();
    audioContext.resume();
    audioControl = new AbortController();
    pullAudio(audioControl.signal, note, audioContext).catch(function (error) {
      if (error.name !== "AbortError" && note) {
        note.textContent = "Microphone stopped.";
      }
    });
  }

  function setActive(active) {
    const image = document.getElementById("cameraView");
    const audio = document.getElementById("cameraAudio");
    const note = document.getElementById("cameraNote");
    if (!image) {
      return;
    }
    if (!active) {
      image.removeAttribute("src");
      stopAudio();
      if (note) {
        note.textContent = "";
      }
      return;
    }
    if (note) {
      note.textContent = "Connecting to camera on port 47690…";
    }
    image.onerror = function () {
      if (note) {
        note.textContent =
          "Camera stream failed (port 47690). Stay signed in, hard-refresh, and open this tab again.";
      }
    };
    image.onload = function () {
      if (note) {
        note.textContent = "";
      }
    };
    image.src = streamUrl(47690, "/camera.mjpg") + "?t=" + Date.now();
    if (audio && audio.checked && note) {
      startAudio(note);
    }
  }

  function bind() {
    const audio = document.getElementById("cameraAudio");
    const note = document.getElementById("cameraNote");
    if (!audio || audio.dataset.bound === "1") {
      return;
    }
    audio.dataset.bound = "1";
    const volume = document.getElementById("cameraVolume");
    if (volume && volume.dataset.bound !== "1") {
      volume.dataset.bound = "1";
      volume.addEventListener("input", function () {
        const percent = Number(volume.value);
        gainHoldUntil = Date.now() + 1500;
        applyPercent(percent);
        postGain(percent);
      });
    }
    audio.addEventListener("change", function () {
      const tab = document.querySelector('.tab[data-tab="camera"]');
      if (!tab || !tab.classList.contains("active")) {
        return;
      }
      if (audio.checked && note) {
        startAudio(note);
      } else {
        stopAudio();
      }
    });
    global.addEventListener("pagehide", function () {
      setActive(false);
    });
  }

  bind();
  global.WattcycleCamera = { setActive: setActive };
})(window);
