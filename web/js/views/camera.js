(function (global) {
  let audioControl = null;
  let audioContext = null;
  let audioGain = null;
  let playAt = 0;

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
    const response = await fetch(streamUrl(47691, "/mic.pcm"), {
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
      const usable = merged.length - (merged.length % 4);
      if (usable > 0) {
        playPcm(context, new Int16Array(merged.buffer, 0, usable / 2));
      }
      pending = merged.slice(usable);
    }
  }

  function startAudio(note) {
    stopAudio();
    audioContext = new AudioContext({ sampleRate: 16000 });
    audioGain = audioContext.createGain();
    audioGain.gain.value = 6;
    audioGain.connect(audioContext.destination);
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
