#pragma once

namespace wattcycle::audio {

/// Shared microphone playback level for the web slider and the panel slider.
class IMicGain {
 public:
  virtual ~IMicGain() = default;

  /// 0 is mute, 100 is the previous fixed browser gain.
  virtual int percent() const = 0;
  /// Clamp to 0..100 and store. Playback applies it; the codec register stays put.
  virtual void setPercent(int percent) = 0;
};

}  // namespace wattcycle::audio
