#include "Engine.h"
#include <unordered_map>
namespace malle {
float Engine::value(const char *id) const { return p[size_t(index(id))]; }
void Engine::prepare(double rate, int block, int channels) {
  (void)profiles();
  sr = float(rate);
  chs = channels;
  p = defaults();
  for(int slot=0;slot<crowdSlots;++slot) {
    int j=0;for(auto field : {"root","loop","fade","level"})
      crowdIndices[size_t(slot)][size_t(j++)]=index(crowdParameter(slot,field));
  }
  pitchEngine.presetDefault(chs, sr);
  creativeEngine.presetDefault(chs, sr);
  latencySamples =
      2 * (pitchEngine.inputLatency() + pitchEngine.outputLatency());
  for (int c = 0; c < 2; ++c) {
    pitchIn[c].assign(std::max(block, 64), 0);
    pitchOut[c].assign(std::max(block, 64), 0);
    rawInput[c].assign(std::max(block, 64), 0);
    work[c].assign(std::max(block, 64), 0);
    tapInput[c].assign(std::max(block, 64), 0);
    auto &x = channel[c];
    x.dry.prepare(latencySamples + 8);
    x.filteredDry.prepare(latencySamples + 8);
    x.creativeDry.prepare(latencySamples + 8);
    x.inputTap.prepare(latencySamples + 8);
    x.throwReverb.prepare(sr);
    for (auto &r : x.reverbs)
      r.prepare(sr);
    x.side.prepare(latencySamples + 8);
    x.doubler.prepare(int(sr * .2));
    for (auto &d : x.delays)
      d.prepare(int(sr * 9));
    for (int b = 0; b < 32; ++b)
      x.reson[b].set(2, 120 * std::pow(150.f, b / 31.f), 8, 0, sr);
    for (int b = 0; b < 16; ++b) {
      float f = 100 * std::pow(120.f, b / 15.f);
      x.voice[b].set(2, f, 2, 0, sr);
      x.carrier[b].set(2, f, 2, 0, sr);
    }
  }
  static const auto testBank = [] {
    constexpr float sr = 22050;
    std::array<Sample, 7> bank;
    for (int texture = 0; texture < 7; ++texture) {
      auto &test = bank[texture];
      test.sampleRate = sr;
      test.name =
          "Original synthetic test crowd " + std::to_string(texture + 1);
      test.left.resize(size_t(sr * 2));
      test.right.resize(test.left.size());
      unsigned seed = 76543 + texture;
      for (size_t i = 0; i < test.left.size(); ++i) {
        float t = float(i) / sr, l = 0, r = 0;
        for (int v = 0; v < 5 + texture; ++v) {
          float f = 110 * std::pow(2.f, (v - 3) * (.006f + texture * .002f));
          for (int h = 1; h < 12; ++h) {
            float a = 1.f / std::pow(float(h), .7f + texture * .15f);
            l += a * std::sin(2 * pi * f * h * t + v);
            r += a * std::sin(2 * pi * f * h * t + v + .3f + texture * .1f);
          }
        }
        seed = 1664525 * seed + 1013904223;
        float noise =
            (float(seed >> 8) / 8388608.f - 1) * (.01f + texture * .005f);
        test.left[i] = l * .02f + noise;
        test.right[i] = r * .02f - noise;
      }
    }
    return bank;
  }();
  synthetic = testBank;
  setParameters(p);
  reset();
}
void Engine::reset() {
  for(auto& m : levelMatch) m = {};
  throwSendGain = 0;
  previousDelay.fill(0);targetDelay.fill(0);delayBlend.fill(1);
  pitchEngine.reset();
  creativeEngine.reset();
  for (auto &x : channel) {
    x.hp.reset();
    x.lp.reset();
    x.correct.reset();
    x.dsLow.reset();
    x.dsHigh.reset();
    x.airLow.reset();
    x.airHigh.reset();
    x.satTone.reset();
    x.colourLow.reset();x.colourHigh.reset();x.crowdLow.reset();x.crowdHigh.reset();
    x.airEnv = {};
    x.airBroadEnv = {};
    x.throwReverb.reset();
    for (auto &f : x.eq)
      f.reset();
    for (auto &f : x.eqDetect)
      f.reset();
    for (auto &e : x.eqEnv)
      e.e = 0;
    for (auto &f : x.cross)
      f.reset();
    for (auto &f : x.reson)
      f.reset();
    for (auto &e : x.resonEnv)
      e.e = 0;
    x.resonGain.fill(0);
    for (auto &f : x.voice)
      f.reset();
    for (auto &f : x.carrier)
      f.reset();
    for (auto &e : x.vocEnv)
      e.e = 0;
    for (auto &d : x.delays) {
      std::fill(d.data.begin(), d.data.end(), 0);
      d.pos = 0;
    }
    for (auto &r : x.reverbs)
      r.reset();
    for (auto *d : {&x.dry, &x.filteredDry, &x.creativeDry, &x.inputTap,
                    &x.side, &x.doubler}) {
      std::fill(d->data.begin(), d->data.end(), 0);
      d->pos = 0;
    }
  }
  comp = {};
  pumpComp = {};
  deessEnv = {};
  duckEnv = {};
  stadiumEnv = {};
  pitchHistory.fill(0);
  pitchPos = pitchHop = downCounter = 0;
  pitchLow = pitchMidi = pitchConfidence = correction = 0;
  samplePos = phase = freePpq = 0;
  crowdPlayer.reset();
  clock = 0;
  throwStart = throwEnd = -1;
  throwLag = throwRemaining = 0;
  lastThrow = false;
  inactivity.fill(0);
  totalInactive = 0;
  smoothIn = gain(value("input"));
  smoothOut = gain(value("output"));
  smoothMix = value("mix");
  tuneMix = 0;
  creativeMix = 0;
  routeGain = 1;
  routeChanging = false;
  lastKill = false;
  lastPreview = false;
  killedAt = -1000000000;
  for (int i = 0; i < 11; ++i)
    order[i] = i;
  pendingOrder = order;
}
void Engine::setParameters(const Values &values) {
  p = values;
  bool nextMonitoring=value("monitor.on")>.5f;
  if(nextMonitoring!=monitoring) {
    // Reset only preallocated pitch/alignment history, retaining FX tails.
    pitchEngine.reset();creativeEngine.reset();tuneMix=creativeMix=0;
    pitchHistory.fill(0);pitchConfidence=correction=0;
    for(auto& x:channel)for(auto* d:{&x.dry,&x.filteredDry,&x.creativeDry,&x.inputTap,&x.side}) {
      std::fill(d->data.begin(),d->data.end(),0);d->pos=0;
    }
    throwStart=-1;throwRemaining=0;lastThrow=false;
  }
  monitoring=nextMonitoring;
  for (auto &x : channel) {
    x.hp.set(1, value("hp"), .707f, 0, sr);
    x.lp.set(0, value("lp"), .707f, 0, sr);
    x.correct.set(3, value("correct.freq"), value("correct.q"),
                  value("correct.gain"), sr);
    x.dsLow.set(1, std::min(value("deess.low"), value("deess.high") * .9f),
                .707f, 0, sr);
    x.dsHigh.set(0, value("deess.high"), .707f, 0, sr);
    x.airLow.set(1, 2500, .707f, 0, sr);
    x.airHigh.set(1, 9000, .707f, 0, sr);
    x.satTone.set(0, value("sat.tone"), .707f, 0, sr);
    x.colourLow.set(4,250,.707f,-6*value("klang.colour"),sr);
    x.colourHigh.set(5,4500,.707f,8*value("klang.colour"),sr);
    x.crowdLow.set(1,value("stadium.low"),.707f,0,sr);
    x.crowdHigh.set(0,value("stadium.high"),.707f,0,sr);
    for (int b = 0; b < 4; ++b) {
      int start = index("eq0.freq") + b * 6;
      auto g = [&](int i) { return p[size_t(start + i)]; };
      x.eq[b].set(3 + int(g(3)), g(0), g(2), g(1), sr);
      x.eqDetect[b].set(2, g(0), g(2), 0, sr);
    }
    float last = 40;
    for (int b = 0; b < 3; ++b) {
      float f =
          std::clamp(p[size_t(index("mb.cross0") + b)], last * 1.2f, sr * .4f);
      x.cross[b].set(0, f, .707f, 0, sr);
      last = f;
    }
    for (int b = 0; b < 5; ++b) {
      int start = index("room.on") + b * 12;
      x.delayHP[b].set(1, p[size_t(start + 7)], .707f, 0, sr);
      x.delayLP[b].set(0, p[size_t(start + 8)], .707f, 0, sr);
    }
  }
}
void Engine::detectPitch() {
  std::array<float, 1024> a{};
  for (int i = 0; i < 1024; ++i)
    a[i] = pitchHistory[size_t((pitchPos + i) % 1024)];
  float ds = sr / float(std::max(1, int(sr / 12000)));
  int low = std::max(2, int(ds / 1000)), hi = std::min(510, int(ds / 65));
  std::array<float, 512> diff{};
  float sum = 0;
  int best = -1;
  for (int tau = 1; tau <= hi; ++tau) {
    float d = 0;
    for (int i = 0; i < 512; ++i) {
      float z = a[i] - a[i + tau];
      d += z * z;
    }
    sum += d;
    diff[tau] = sum > 1e-10f ? d * tau / sum : 1;
    if (tau > low + 1 && diff[tau - 1] < .18f &&
        diff[tau - 1] < diff[tau - 2] && diff[tau - 1] <= diff[tau]) {
      best = tau - 1;
      break;
    }
  }
  if (best > 0) {
    float y0 = diff[best - 1], y1 = diff[best], y2 = diff[best + 1],
          den = y0 - 2 * y1 + y2;
    float delta = std::abs(den) > 1e-8f ? .5f * (y0 - y2) / den : 0;
    float hz = ds / (best + std::clamp(delta, -.5f, .5f));
    pitchMidi = 69 + 12 * std::log2(hz / 440);
    pitchConfidence = 1 - y1;
  } else
    pitchConfidence *= .8f;
  meters.pitch.store(pitchMidi);
  meters.confidence.store(pitchConfidence);
}
void Engine::process(float *const *data, int channels, int count,
                     const float *const *sc, int scChannels, Transport tr,
                     bool hostBypass) {
  if (count <= 0)
    return;
  const int latencySamples = latency();
  const bool modern = value("quality.on") > .5f;
  const bool character = value("macro.v2.on") > .5f;
  std::array<bool,4> matching{};
  std::array<float,6> trims{};
  int si=0;
  for(auto section : {"druck","klang","waerme","breite","raum","bewegung"}) {
    trims[si]=p[size_t(index(std::string(section)+".on"))]>.5f ? gain(p[size_t(index(std::string(section)+".trim"))]) : 1.f;
    if(si<4)matching[si]=p[size_t(index(std::string(section)+".match.on"))]>.5f;
    ++si;
  }
  const auto at = [&](std::string_view id) {
    return p[size_t(index(id))];
  }; // block-only lookups
  struct CP {
    float t, r, a, rel, k, m, mix;
    bool on;
  };
  std::array<CP, 7> cp{};
  int ci = 0;
  for (int k = 0; k < 3; ++k) {
    int start = index(k == 0 ? "fet.on" : k == 1 ? "level.on" : "parallel.on");
    auto a = [&](int i) { return p[size_t(start + i)]; };
    cp[ci++] = {a(1), a(2), a(3), a(4), a(5), gain(a(6)), a(7), a(0) > .5f};
  }
  for (int b = 0; b < 4; ++b) {
    int start = index("mb0.threshold") + b * 5;
    auto a = [&](int i) { return p[size_t(start + i)]; };
    cp[ci++] = {a(0), a(1), a(2), a(3), 6, gain(a(4)), 1, at("mb.on") > .5f};
  }
  struct FX {
    bool on, sync;
    float send, ret, time, beats, fb, width, tail, fade;
  };
  std::array<FX, 5> fx{};
  for (int b = 0; b < 5; ++b) {
    int start = index("room.on") + b * 12;
    auto a = [&](int i) { return p[size_t(start + i)]; };
    fx[b] = {a(0) > .5f, a(4) > .5f, a(1), a(2),  a(3),
             a(5),       a(6),       a(9), a(10), a(11)};
  }
  for (int c = 0; c < channels; ++c) {
    channel[c].reverbs[0].configure(fx[0].width, std::min(1.5f, fx[0].tail),
                                    6500, fx[0].fb);
    channel[c].reverbs[1].configure(modern ? std::array<float,3>{.18f,.48f,.90f}[size_t(std::clamp(int(value("reverb.type")),0,2))] : fx[3].width,
      modern ? value("reverb.decay") : fx[3].tail, 6500, fx[3].fb,modern);
    if(modern)channel[c].reverbs[0].configure(.18f,value("room.decay"),6500,fx[0].fb,true);
  }
  std::array<float, 4> eqDynamic{}, eqThreshold{};
  for (int b = 0; b < 4; ++b) {
    int start = index("eq0.freq") + b * 6;
    eqDynamic[b] = p[size_t(start + 4)];
    eqThreshold[b] = p[size_t(start + 5)];
  }
  // Copy scalar parameters once per block: the sample loop uses no
  // strings/maps/allocations.
  const float inTarget = gain(value("input")),
              outTarget = gain(value("output")), mixTarget = value("mix"),
              fxMix = value("fx.mix"), smoothing = coef(10, sr);
  const bool tune = value("tune.on") > .5f, eqOn = value("eq.on") > .5f,
             inputOn = value("inputEQ.on") > .5f,
             dsOn = value("deess.on") > .5f, resOn = value("res.on") > .5f,
             airOn = value("air.on") > .5f, satOn = value("sat.on") > .5f,
             doubleOn = value("double.on") > .5f,
             stadiumOn = value("stadium.on") > .5f,
             bypass = hostBypass || value("bypass") > .5f;
  const float dsAmount = value("deess.amount"),
              dsThreshold = value("deess.threshold"),
              dsListen = value("deess.listen"), resAmount = value("res.amount"),
              resMax = value("res.max"), resFocus = value("res.focus"),
              resSelect = value("res.select"),
              resA = coef(value("res.attack"), sr),
              resR = coef(value("res.release"), sr),
              resDelta = value("res.delta"), airP = value("air.presence"),
              airH = value("air.high"), airGuard = value("air.guard"),
              drive = gain(value("sat.drive")), satMix = value("sat.mix"),
              satOut = gain(value("sat.output")), satType = value("sat.type"),
              doubleMix = value("double.mix"),
              doubleTime = value("double.time"),
              doublePitch = value("double.pitch"),
              doubleWidth = value("double.width"),
              stadiumMix = value("stadium.mix"),
              stadiumA = coef(value("stadium.attack"), sr),
              stadiumR = coef(value("stadium.release"), sr),
              stadiumVoc = value("stadium.vocoder"),
              stadiumWidth = value("stadium.width"),
              stadiumRough = value("stadium.rough"),
              stadiumSize = value("stadium.size"),
              sampleRoot = p[size_t(crowdIndices[size_t(std::clamp(int(value("stadium.slot")),0,6))][0])],
              sampleLoop = p[size_t(crowdIndices[size_t(std::clamp(int(value("stadium.slot")),0,6))][1])],
              sampleFade = p[size_t(crowdIndices[size_t(std::clamp(int(value("stadium.slot")),0,6))][2])],
              sampleGain = gain(p[size_t(crowdIndices[size_t(std::clamp(int(value("stadium.slot")),0,6))][3])]);
  const float crowdPitch=std::pow(2.f,value("stadium.pitch")/12.f);
  const int voices = int(value("double.voices")),
            root = int(value("tune.root")), scale = int(value("tune.scale"));
  std::array<bool, 12> allowed{};
  for (int i = 0; i < 12; ++i)
    allowed[i] = p[size_t(index("note.0") + i)] > .5f;
  const float tuneAmount = value("tune.amount"),
              tuneSpeed = value("tune.speed"), human = value("tune.human"),
              pitchShift = value("pitch"), formant = value("formant");
  const bool preserve = value("formant.preserve") > .5f;
  const bool external = value("duck.source") > .5f,
             pumpOn = value("pump.on") > .5f, pumpSC = value("pump.mode") > .5f;
  const float duckAmount = value("duck.amount"), duckDry = value("duck.dry"),
              duckA = coef(value("duck.attack"), sr),
              duckR = coef(value("duck.release"), sr),
              pumpDepth = value("pump.depth"), pumpBeats = value("pump.beats"),
              pumpPhase = value("pump.phase"), pumpShape = value("pump.shape"),
              pumpTarget = value("pump.target"), stutter = value("stutter"),
              globalTail = value("tail"), globalFade = value("fade"),
              delayToVerb = value("delay.reverb");
  const float duckThreshold=gain(value("duck.threshold"));
  const float pumpThreshold = value("pump.threshold"),
              pumpRatio = value("pump.ratio"),
              pumpAttack = value("pump.attack"),
              pumpRelease = value("pump.release");
  const bool smoothCrowdLoop=value("stadium.loopSmooth")>.5f;
  bool trigger = value("throw.trigger") > .5f;
  double bpm = std::clamp(tr.bpm, 20., 400.), beatsPerSample = bpm / (60 * sr);
  if (trigger && !lastThrow) {
    double delay = value("throw.offset") * .001 * sr;
    float q = value("throw.quantize");
    if (q > 0 && tr.hasPosition) {
      double atBeat = tr.ppq + delay * beatsPerSample;
      delay += (std::ceil(atBeat / q - 1e-9) * q - atBeat) / beatsPerSample;
    }
    throwLag = latencySamples + static_cast<long long>(delay);
    throwStart = clock + throwLag;
    throwEnd = -1;
    throwRemaining = static_cast<long long>(value("throw.window") * .001 * sr);
  }
  if (!trigger && lastThrow)
    throwEnd = clock + throwLag; // close aligned send only; never clear the tail
  lastThrow = trigger;
  const Sample *samp = samples[size_t(std::clamp(int(value("stadium.slot")),0,6))].load(std::memory_order_acquire);
  if (!samp || samp->left.empty())
    samp = &synthetic[size_t(std::clamp(int(value("stadium.texture")), 0, 6))];

  const bool creative = value("creative.on") > .5f,
             wideDeess = value("deess.mode") > .5f,
             duckOn = value("duck.on") > .5f;
  const bool parInput = value("parallel.tap") < .5f;
  const int throwType = int(value("throw.type")),
            feel = int(value("throw.feel")),
            verbType = int(value("throw.verb.type"));
  const float throwFeel = feel == 1   ? 1.5f
                          : feel == 2 ? 2.f / 3
                                      : 1.f,
              verbAmount = value("throw.verb.amount");
  const bool verbOn = value("throw.verb.on") > .5f,
             kill = value("throw.kill") > .5f, preview = audition.load();
  if(preview && !lastPreview) crowdPlayer.reset();
  lastPreview=preview;
  if (kill && !lastKill) {
    killedAt = clock;
    throwRemaining = 0;
    throwStart = -1;
    for (auto &z : channel) {
      z.throwReverb.reset();
      z.delayHP[4].reset();
      z.delayLP[4].reset();
    }
  }
  lastKill = kill;
  for (int c = 0; c < channels; ++c)
    channel[c].throwReverb.configure(verbType == 0   ? .15f
                                     : verbType == 1 ? .55f
                                                     : .95f,
                                     verbType == 0   ? 1.f
                                     : verbType == 1 ? 2.5f
                                                     : 5.f,
                                     verbType == 1 ? 9000 : 5500, .8f, modern);
  if(modern)for(int c=0;c<channels;++c)channel[c].throwReverb.configure(verbType==0?.18f:verbType==1?.48f:.9f,value("throw.verb.decay"),5500,.8f,true);
  std::array<int, 11> requested{};
  std::array<bool, 11> seen{};
  bool valid = true;
  for (int i = 0; i < 11; ++i) {
    int v = int(p[size_t(index("route.0") + i)]);
    if (v < 0 || v >= 11 || seen[size_t(std::clamp(v, 0, 10))]) {
      valid = false;
      break;
    }
    seen[size_t(v)] = true;
    requested[i] = v;
  }
  if (valid && routeChanging && requested == order)
    routeChanging = false;
  if (valid && requested != order) {
    pendingOrder = requested;
    routeChanging = true;
  }
  float maxIn = 0, maxOut = 0, maxSC = 0;
  auto compress = [&](int k, float det) {
    auto &q = cp[k];
    return q.on ? comp[k].tickGain(det, q.t, q.r, q.a, q.rel, q.k, sr) * q.m
                : 1.f;
  };
  auto direct = [&](int module, std::array<float, 2> &x) {
    if (module == 0) {
      if (inputOn)
        for (int c = 0; c < channels; ++c) {
          auto &z = channel[c];
          x[c] = z.correct.tick(z.lp.tick(z.hp.tick(x[c])));
        }
      return;
    }
    if (module == 2 || module == 3) {
      int k = module - 2;
      float det = 0;
      for (int c = 0; c < channels; ++c)
        det = std::max(det, std::abs(x[c]));
      float g = compress(k, det);
      for (int c = 0; c < channels; ++c)
        x[c] *= 1 + cp[k].mix * (g - 1);
      return;
    }
    if (module == 4) {
      std::array<float, 2> ds{};
      float det = 0;
      for (int c = 0; c < channels; ++c) {
        ds[c] = channel[c].dsHigh.tick(channel[c].dsLow.tick(x[c]));
        det = std::max(det, std::abs(ds[c]));
      }
      float over = std::max(
          0.f, db(deessEnv.tick(det, coef(1, sr), coef(70, sr))) - dsThreshold);
      float red = std::min(15.f, over * dsAmount);
      meters.deess.store(dsOn ? red : 0);
      if (dsOn)
        for (int c = 0; c < channels; ++c) {
          if (dsListen > .5f)
            x[c] = ds[c];
          else if (wideDeess)
            x[c] *= gain(-red);
          else
            x[c] -= ds[c] * (1 - gain(-red));
        }
      return;
    }
    if (module == 5) {
      if (eqOn)
        for (int c = 0; c < channels; ++c)
          for (int b = 0; b < 4; ++b) {
            auto &z = channel[c];
            float band = z.eqDetect[b].tick(x[c]),
                  env = z.eqEnv[b].tick(band, coef(5, sr), coef(120, sr));
            x[c] = z.eq[b].tick(x[c]) +
                   band * (gain(eqDynamic[b] *
                                std::clamp((db(env) - eqThreshold[b]) / 12, 0.f,
                                           1.f)) -
                           1);
          }
      if(eqOn && character)for(int c=0;c<channels;++c)
        x[c]=channel[c].colourHigh.tick(channel[c].colourLow.tick(x[c]));
      return;
    }
    if (module == 6) {
      if (!cp[3].on)
        return;
      std::array<std::array<float, 4>, 2> bands{};
      for (int c = 0; c < channels; ++c) {
        float low[3];
        for (int b = 0; b < 3; ++b)
          low[b] = channel[c].cross[b].tick(x[c]);
        bands[c] = {low[0], low[1] - low[0], low[2] - low[1], x[c] - low[2]};
        x[c] = 0;
      }
      for (int b = 0; b < 4; ++b) {
        float det = 0;
        for (int c = 0; c < channels; ++c)
          det = std::max(det, std::abs(bands[c][b]));
        float g = compress(3 + b, det);
        for (int c = 0; c < channels; ++c)
          x[c] += bands[c][b] * g;
      }
      return;
    }
    if (module == 7) {
      for (int c = 0; c < channels; ++c) {
        auto &z = channel[c];
        std::array<float, 32> rb{}, env{};
        for (int b = 0; b < 32; ++b) {
          rb[b] = z.reson[b].tick(x[c]);
          env[b] = z.resonEnv[b].tick(rb[b], resA, resR);
          if (c == 0)
            meters.spectrum[b].store(env[b]);
        }
        float delta = 0, maxRed = 0;
        for (int b = 1; b < 31; ++b) {
          float neighbor = (env[b - 1] + env[b + 1]) * .5f,
                prominence = db(env[b] / std::max(1e-7f, neighbor)),
                weight = std::clamp(1 + resFocus * (b / 15.f - 1), 0.f, 2.f);
          float red =
              resOn ? std::clamp((prominence - resSelect) * resAmount * weight,
                                 0.f, resMax)
                    : 0;
          if (env[b] < 1e-5f)
            red = 0;
          float target = 1 - gain(-red),
                a = target > z.resonGain[b] ? resA : resR;
          z.resonGain[b] = a * z.resonGain[b] + (1 - a) * target;
          delta += rb[b] * z.resonGain[b] * .35f;
          maxRed = std::max(maxRed, red);
          if (c == 0)
            meters.reduction[b].store(red);
        }
        if (resOn)
          x[c] = resDelta > .5f ? delta : x[c] - delta;
        meters.resonance.store(maxRed);
      }
      return;
    }
    if (module == 8) {
      if (airOn)
        for (int c = 0; c < channels; ++c) {
          auto &z = channel[c];
          float a = z.airLow.tick(x[c]), h = z.airHigh.tick(x[c]);
          float over =
              std::max(0.f, db(z.airEnv.tick(h, coef(2, sr), coef(80, sr))) -
                                dsThreshold);
          float noise = std::clamp(
              (db(z.airBroadEnv.tick(x[c], coef(3, sr), coef(100, sr))) + 65) /
                  20,
              0.f, 1.f);
          float guard = (1 - airGuard * std::clamp(over / 12, 0.f, 1.f)) *
                        (1 - airGuard + airGuard * noise);
          x[c] += guard * (a * airP * .7f + h * airH);
        }
      return;
    }
    if (module == 9) {
      if (satOn)
        for (int c = 0; c < channels; ++c) {
          float driven = x[c] * drive,
                y = satType < .5f    ? std::tanh(driven)
                    : satType < 1.5f ? std::tanh(driven + .2f) - std::tanh(.2f)
                                     : std::clamp(driven, -1.f, 1.f);
          y = channel[c].satTone.tick(y / std::max(1.f, std::sqrt(drive)));
          x[c] = (x[c] * (1 - satMix) + y * satMix) * satOut;
        }
      return;
    }
  };
  int offset = 0;
  while (offset < count) {
    int n = std::min(count - offset, int(work[0].size()));
    for (int i = 0; i < n; ++i) {
      smoothIn = smoothing * smoothIn + (1 - smoothing) * inTarget;
      for (int c = 0; c < channels; ++c) {
        float x = data[c][offset + i];
        if (!std::isfinite(x))
          x = 0;
        rawInput[c][i] = x;
        work[c][i] = tapInput[c][i] = x * smoothIn;
        maxIn = std::max(maxIn, std::abs(x * smoothIn));
      }
    }
    for (int module : order) {
      int sectionId=module==2||module==3||module==6?0:module==9?2:1;
      bool last=true,found=false;
      for(int node:order){
        if(node==module){found=true;continue;}
        if(!found||node==1||node==10||node==4)continue;
        int group=node==2||node==3||node==6?0:node==9?2:1;
        if(group==sectionId)last=false;
      }
      if (module == 1 || module == 10) {
        if(monitoring) continue; // Explicit recording mode: no pitch processing or alignment delays.
        bool tuningNode = module == 1, enabled = tuningNode ? tune : creative;
        auto &pitch = tuningNode ? pitchEngine : creativeEngine;
        for (int i = 0; i < n; ++i) {
          float mono = 0;
          for (int c = 0; c < channels; ++c) {
            pitchIn[c][i] = work[c][i];
            mono += work[c][i];
          }
          if (tuningNode) {
            mono /= channels;
            pitchLow += .35f * (mono - pitchLow);
            if (++downCounter >= std::max(1, int(sr / 12000))) {
              downCounter = 0;
              pitchHistory[pitchPos] = pitchLow;
              pitchPos = (pitchPos + 1) % 1024;
              if (++pitchHop >= 128) {
                pitchHop = 0;
                detectPitch();
              }
            }
          }
        }
        float targetCorrection = 0;
        if (tuningNode && tune && pitchConfidence > .75f) {
          float best = 100;
          int target = int(std::round(pitchMidi));
          for (int note = int(pitchMidi) - 12; note <= int(pitchMidi) + 12;
               ++note) {
            int degree = ((note - root) % 12 + 12) % 12;
            bool ok =
                scale == 0 ||
                (scale == 1 &&
                 (degree == 0 || degree == 2 || degree == 4 || degree == 5 ||
                  degree == 7 || degree == 9 || degree == 11)) ||
                (scale == 2 &&
                 (degree == 0 || degree == 2 || degree == 3 || degree == 5 ||
                  degree == 7 || degree == 8 || degree == 10)) ||
                (scale == 3 && allowed[(note % 12 + 12) % 12]);
            float dist = std::abs(note - pitchMidi);
            if (ok && dist < best) {
              best = dist;
              target = note;
            }
          }
          targetCorrection =
              (target - pitchMidi) * tuneAmount * (1 - human * .35f);
        }
        if (tuningNode) {
          float tc = std::pow(coef(std::max(.01f, tuneSpeed), sr), float(n));
          correction = tc * correction + (1 - tc) * targetCorrection;
        }
        pitch.setTransposeSemitones(
            enabled ? (tuningNode ? correction : pitchShift) : 0);
        pitch.setFormantSemitones(enabled && !tuningNode ? formant : 0,
                                  preserve);
        pitch.setFormantBase(pitchConfidence > .5f
                                 ? 440 * std::pow(2.f, (pitchMidi - 69) / 12) /
                                       sr
                                 : 200 / sr);
        float *ins[] = {pitchIn[0].data(), pitchIn[1].data()};
        float *outs[] = {pitchOut[0].data(), pitchOut[1].data()};
        pitch.process(ins, n, outs, n);
        float &blend = tuningNode ? tuneMix : creativeMix;
        for (int i = 0; i < n; ++i) {
          blend = smoothing * blend + (1 - smoothing) * (enabled ? 1.f : 0.f);
          for (int c = 0; c < channels; ++c) {
            auto &delay =
                tuningNode ? channel[c].filteredDry : channel[c].creativeDry;
            float dry = delay.read(float(latencySamples / 2));
            delay.push(pitchIn[c][i]);
            work[c][i] = dry * (1 - blend) + pitchOut[c][i] * blend;
          }
        }
      } else
        for (int i = 0; i < n; ++i) {
          std::array<float, 2> x{};
          for (int c = 0; c < channels; ++c)
            x[c] = work[c][i];
          float before=0, after=0;
          for(int c=0;c<channels;++c)before+=x[c]*x[c];
          direct(module, x);
          for(int c=0;c<channels;++c)after+=x[c]*x[c];
          bool listen=(module==4 && dsListen>.5f)||(module==7 && resDelta>.5f);
          bool nodeActive=module==0?inputOn:module==2?cp[0].on:module==3?cp[1].on:module==4?(character && dsOn):module==5?eqOn:module==6?cp[3].on:module==7?resOn:module==8?airOn:module==9?satOn:false;
          float manualGain=module==2?cp[0].m:module==3?cp[1].m:module==9?satOut:1.f;
          float compensation=levelMatch[size_t(module)].tick(before*manualGain*manualGain/channels,after/channels,sr,matching[size_t(sectionId)] && nodeActive && !listen);
          // One trim per section, after its final node in the actual reordered chain.
          if(last && module!=4)compensation*=trims[size_t(sectionId)];
          for(int c=0;c<channels;++c)x[c]*=compensation;
          for (int c = 0; c < channels; ++c)
            work[c][i] = x[c];
        }
    }
    for (int i = 0; i < n; ++i, ++clock) {
      int atSample = offset + i;
      smoothOut = smoothing * smoothOut + (1 - smoothing) * outTarget;
      smoothMix = smoothing * smoothMix + (1 - smoothing) * mixTarget;
      std::array<float, 2> x{}, dry{}, tap{};
      float detector = 0, scDet = 0;
      for (int c = 0; c < channels; ++c) {
        auto &z = channel[c];
        dry[c] = monitoring ? rawInput[c][i] : z.dry.read(float(latencySamples));
        z.dry.push(rawInput[c][i]);
        tap[c] = monitoring ? tapInput[c][i] : z.inputTap.read(float(latencySamples));
        z.inputTap.push(tapInput[c][i]);
        x[c] = work[c][i];
        detector = std::max(detector, std::abs(x[c]));
        float scIn =
            scChannels && sc ? sc[std::min(c, scChannels - 1)][atSample] : 0;
        float delayed = monitoring ? scIn : z.side.read(float(latencySamples));
        z.side.push(scIn);
        scDet = std::max(scDet, std::abs(delayed));
      }
      maxSC = std::max(maxSC, scDet);
      float parDet = 0;
      for (int c = 0; c < channels; ++c)
        parDet = std::max(parDet, std::abs(parInput ? tap[c] : x[c]));
      float parGain = compress(2, parDet);
      auto source = x;
      float env = stadiumEnv.tick(detector, stadiumA, stadiumR),
            duck = duckEnv.tick(external ? scDet : detector, duckA, duckR);
      float duckG = duckOn ? 1 / (1 + duck * 12 * duckAmount) : 1,
            dryDuck = duckOn ? 1 / (1 + duck * 12 * duckDry) : 1;
      if(character && duckOn) {
        // 0 = full return, 100% = silence while a vocal is above threshold.
        // The existing attack/release envelope brings tails back in pauses.
        float activity=std::clamp(duck/std::max(1e-6f,duckThreshold),0.f,1.f);
        duckG=1-duckAmount*activity;dryDuck=1-duckDry*activity;
      }
      double beat = tr.hasPosition
                        ? tr.ppq + (atSample - latencySamples) * beatsPerSample
                        : freePpq;
      freePpq += beatsPerSample;
      float ph = float(std::fmod(beat / pumpBeats + pumpPhase, 1.));
      if (ph < 0)
        ph += 1;
      float pump = 1;
      if (pumpOn) {
        float shape = std::pow(.5f - .5f * std::cos(2 * pi * ph), pumpShape),
              scGain = pumpSC
                           ? pumpComp.tickGain(scDet, pumpThreshold, pumpRatio,
                                               pumpAttack, pumpRelease, 6, sr)
                           : shape;
        pump = 1 - pumpDepth * (1 - scGain);
      }
      if (stutter > .5f)
        pump *= ph < .5f ? 1 : 0;
      if (detector > 1e-5f)
        totalInactive = 0;
      else
        totalInactive += 1 / sr;
      float globalGate =
          std::clamp((globalTail - totalInactive) / globalFade, 0.f, 1.f);
      auto carrier = crowdPlayer.next(samp,sr,
          crowdPitch*std::pow(2.f,-sampleRoot/12.f)*(1+.04f*stadiumSize),sampleLoop>.5f,
          sampleFade,smoothCrowdLoop,sampleLoop<.5f && env<1e-5f && !preview);
      float carrierL=carrier[0]*sampleGain,carrierR=carrier[1]*sampleGain;
      phase += 1 / sr;
      bool sendThrow =
          throwStart >= 0 && clock >= throwStart && throwRemaining > 0 &&
          (throwEnd < 0 || clock < throwEnd);
      if (sendThrow)
        --throwRemaining;
      std::array<float,2> widthReference{};
      float parallelBefore=0,parallelAfter=0;
      for(int c=0;c<channels;++c)parallelBefore+=x[c]*x[c];
      for(int c=0;c<channels;++c) {
        if(cp[2].on)x[c]+=(parInput?tap[c]:source[c])*parGain*cp[2].mix;
        parallelAfter+=x[c]*x[c];
      }
      float parallelMatch=levelMatch[12].tick(parallelBefore/channels,parallelAfter/channels,sr,character && matching[0] && cp[2].on);
      for (int c = 0; c < channels; ++c) {
        auto &z = channel[c];
        x[c]*=parallelMatch;
        widthReference[c]=x[c];
        z.doubler.push(source[c]);
        if (doubleOn) {
          float doubled = 0;
          for (int v = 0; v < voices; ++v) {
            float sign = v % 2 ? 1.f : -1.f, pan = channels == 1 ? 1.f
                                                   : c == 0
                                                       ? 1 - sign * doubleWidth
                                                       : 1 + sign * doubleWidth;
            float omega = 2 * pi * (character ? .19f+v*.071f : .6f + v * .13f),
                  amplitude = std::min(doubleTime * .6f,
                                       (std::pow(2.f, doublePitch / 1200) - 1) *
                                           1000 / omega);
            float dt = (doubleTime + v * (character?6.7f:2.f) +
                        amplitude * std::sin(float(phase) * omega+(character?v*1.73f:0.f))) *
                       sr * .001f;
            doubled += z.doubler.read(dt) * pan / (character?std::sqrt(float(voices)*2):voices);
          }
          x[c] += doubled * doubleMix;
        }
      }
      float widthBefore=0,widthAfter=0;
      for(int c=0;c<channels;++c){widthBefore+=widthReference[c]*widthReference[c];widthAfter+=x[c]*x[c];}
      float widthGain=levelMatch[11].tick(widthBefore/channels,widthAfter/channels,sr,matching[3] && doubleOn)*trims[3];
      for(int c=0;c<channels;++c){
        auto& z=channel[c]; x[c]*=widthGain;
        if (stadiumOn) {
          float carrier = (c == 0 ? carrierL : carrierR) * stadiumWidth +
                          (carrierL + carrierR) * .5f * (1 - stadiumWidth),
                voc = 0;
          for (int b = 0; b < 16; ++b) {
            float ve = z.vocEnv[b].tick(z.voice[b].tick(source[c]), stadiumA,
                                        stadiumR);
            voc += z.carrier[b].tick(carrier) * std::min(8.f, ve * 35);
          }
          float crowd = carrier * env * (character?8.f:3.f) * (1 - stadiumVoc) + voc * stadiumVoc*(character?2.4f:1.f);
          crowd = std::tanh(crowd * (1 + stadiumRough * 8)) /
                  (1 + stadiumRough * 2);
          if(character) {
            crowd=z.crowdHigh.tick(z.crowdLow.tick(crowd));
            // Softly restrain large carrier peaks without compressing the dry vocal.
            crowd=.25f*std::tanh(crowd*4);
          }
          x[c] += crowd * stadiumMix;
        }
      }
      throwSendGain = modern ? coef(3,sr)*throwSendGain+(1-coef(3,sr))*(sendThrow?1.f:0.f) : (sendThrow?1.f:0.f);
      std::array<float, 2> wet{}, delayFeed{};
      for (int b = 0; b < 5; ++b) {
        auto &q = fx[b];
        std::array<float, 2> filtered{}, send{};
        float dt =
            q.sync ? float(60 / bpm) * q.beats * (b == 4 ? throwFeel : 1) * sr
                   : q.time * .001f * sr;
        if(character) {
          if(targetDelay[b]==0)previousDelay[b]=targetDelay[b]=dt;
          if(std::abs(targetDelay[b]-dt)>.01f){previousDelay[b]=targetDelay[b];targetDelay[b]=dt;delayBlend[b]=0;}
          delayBlend[b]=std::min(1.f,delayBlend[b]+1/(sr*.03f));
        }
        for (int c = 0; c < channels; ++c) {
          auto &z = channel[c];
          float spread = (b == 4 && throwType != 1)
                             ? 1
                             : 1 + (c == 1 ? (modern?.003f:.035f) : (modern?-.003f:-.035f)) * q.width;
          float r = z.delays[b].read(dt * spread);
          if(character && delayBlend[b]<1) {
            float blend=.5f-.5f*std::cos(pi*delayBlend[b]);
            r=z.delays[b].read(previousDelay[b]*spread)*(1-blend)+r*blend;
          }
          if (b == 4 && clock - killedAt < dt * spread)
            r = 0;
          filtered[c] = z.delayLP[b].tick(z.delayHP[b].tick(r));
          if (b == 0 || b == 3)
            filtered[c] = z.reverbs[b == 0 ? 0 : 1].tick(filtered[c]);
          send[c] = x[c] * q.send * (q.on ? 1.f : 0.f);
          if (b == 4)
            send[c] *= throwSendGain;
          if (b == 3)
            send[c] += delayFeed[c] * delayToVerb;
        }
        if (b == 4 && channels == 2) {
          float mono = (send[0] + send[1]) * .5f;
          if (throwType == 0)
            send[0] = send[1] = mono;
          if (throwType == 2) {
            send[0] = mono;
            send[1] = 0;
          }
        }
        float sendPeak = 0;
        for (int c = 0; c < channels; ++c)
          sendPeak = std::max(sendPeak, std::abs(send[c]));
        if (sendPeak > 1e-5f)
          inactivity[b] = 0;
        else
          inactivity[b] += 1 / sr;
        float fade = std::clamp((q.tail - inactivity[b]) / q.fade, 0.f, 1.f);
        for (int c = 0; c < channels; ++c) {
          auto &z = channel[c];
          float feedback =
              filtered[(b == 4 && throwType == 2 && channels == 2) ? 1 - c : c];
          z.delays[b].push(send[c] +
                           ((b == 0 || b == 3) ? 0 : feedback * q.fb * fade));
          float ret = filtered[c];
          if (b == 4) {
            float verb = z.throwReverb.tick(filtered[c] * (verbOn ? 1.f : 0.f));
            ret += verb * verbAmount;
          }
          ret *= q.ret * fade;
          if (b == 1 || b == 2)
            delayFeed[c] += ret;
          wet[c] += ret;
        }
      }
      if (routeChanging)
        routeGain = std::max(0.f, routeGain - 1 / (sr * .01f));
      else
        routeGain = std::min(1.f, routeGain + 1 / (sr * .01f));
      for (int c = 0; c < channels; ++c) {
        float d = x[c] * dryDuck, w = wet[c] * fxMix * duckG * globalGate * trims[4];
        if(pumpOn) { d *= trims[5]; w *= trims[5]; }
        if (pumpTarget < .5f || pumpTarget > 1.5f)
          d *= pump;
        if (pumpTarget > .5f)
          w *= pump;
        float result = ((d + w) * smoothMix + dry[c] * (1 - smoothMix)) *
                       smoothOut * routeGain;
        if (preview)
          result += (c == 0 ? carrierL : carrierR) * .25f;
        if (bypass)
          result = dry[c];
        data[c][atSample] = std::isfinite(result) ? result : 0;
        maxOut = std::max(maxOut, std::abs(data[c][atSample]));
      }
    }
    if (routeChanging && routeGain <= 0) {
      order = pendingOrder;
      routeChanging = false;
    }
    offset += n;
  }
  meters.input.store(maxIn);
  meters.output.store(maxOut);
  meters.fet.store(comp[0].reduction);
  meters.level.store(comp[1].reduction);
  meters.sidechain.store(maxSC);
}
} // namespace malle
