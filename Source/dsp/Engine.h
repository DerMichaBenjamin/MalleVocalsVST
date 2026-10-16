#pragma once
#include "Parameters.h"
#include "CrowdSlots.h"
#include "Primitives.h"
#include "signalsmith-stretch.h"
#include <atomic>
namespace malle {
struct Transport {
  double bpm = 120, ppq = 0;
  bool playing = false, hasPosition = false;
};
struct Meters {
  std::atomic<float> input{0}, output{0}, fet{0}, level{0}, deess{0},
      resonance{0}, pitch{0}, confidence{0}, sidechain{0};
  std::array<std::atomic<float>, 32> spectrum{}, reduction{};
};
class Engine {
public:
  void prepare(double sampleRate, int maxBlock, int channels);
  void reset();
  // Values are already resolved and cached, not dynamically looked up in the
  // callback.
  void setParameters(const Values &values);
  void process(float *const *data, int channels, int count,
               const float *const *sidechain, int scChannels,
               Transport transport, bool bypass = false);
  void setSample(const Sample *s) {
    setSample(0,s);
  }
  void setSample(int slot, const Sample* s) {
    if(slot>=0 && slot<crowdSlots) samples[size_t(slot)].store(s,std::memory_order_release);
  }
  int latency() const { return monitoring ? 0 : latencySamples; }
  Meters meters;
  std::atomic<bool> audition{false};

private:
  float value(const char *id)
      const; // only used for block configuration, indices cached
  float sr = 48000;
  int chs = 2, latencySamples = 0;
  Values p;
  bool monitoring = false;
  std::array<LevelMatch,12> levelMatch;
  float throwSendGain = 0;
  struct Channel {
    Biquad hp, lp, correct, dsLow, dsHigh, airLow, airHigh, satTone;
    std::array<Biquad, 4> eq, eqDetect;
    std::array<Envelope, 4> eqEnv;
    Envelope airEnv, airBroadEnv;
    std::array<Biquad, 3> cross;
    std::array<Biquad, 32> reson;
    std::array<Envelope, 32> resonEnv;
    std::array<float, 32> resonGain{};
    std::array<Biquad, 16> voice, carrier;
    std::array<Envelope, 16> vocEnv;
    std::array<Delay, 5> delays;
    std::array<Biquad, 5> delayHP, delayLP;
    Delay dry, filteredDry, creativeDry, inputTap, side, doubler;
    std::array<Reverb, 2> reverbs;
    Reverb throwReverb;
  };
  std::array<Channel, 2> channel;
  std::array<Compressor, 7> comp;
  Envelope deessEnv, duckEnv, stadiumEnv;
  Compressor pumpComp;
  signalsmith::stretch::SignalsmithStretch<float> pitchEngine, creativeEngine;
  std::array<std::vector<float>, 2> pitchIn, pitchOut, rawInput, work, tapInput;
  std::array<float, 1024> pitchHistory{};
  int pitchPos = 0, pitchHop = 0, downCounter = 0;
  float pitchLow = 0, pitchMidi = 0, pitchConfidence = 0, correction = 0;
  std::array<std::atomic<const Sample *>, crowdSlots> samples{};
  CrowdPlayer crowdPlayer;
  std::array<std::array<int,4>,crowdSlots> crowdIndices{};
  std::array<Sample, 7> synthetic;
  double samplePos = 0, phase = 0, freePpq = 0;
  bool lastThrow = false;
  long long throwStart = -1, throwRemaining = 0, throwEnd = -1, throwLag = 0, clock = 0;
  std::array<float, 5> inactivity{};
  float totalInactive = 0, smoothIn = 1, smoothOut = 1, smoothMix = 1,
        tuneMix = 0;
  std::array<int, 11> order{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      pendingOrder = order;
  float routeGain = 1, creativeMix = 0;
  bool routeChanging = false;
  bool lastKill = false, lastPreview = false;
  long long killedAt = -1000000000;
  void detectPitch();
};
} // namespace malle
