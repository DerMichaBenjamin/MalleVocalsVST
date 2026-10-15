#include "dsp/Engine.h"
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace malle;
void require(bool b, const char *s) {
  if (!b)
    throw std::runtime_error(s);
}
Values neutral() {
  auto p = defaults();
  for (size_t i = 0; i < p.size(); ++i)
    if (parameters()[i].id.find(".on") != std::string::npos)
      p[i] = 0;
  return p;
}
std::vector<float> render(Values p, float hz, int seconds = 3,
                          bool side = false, int impulse = -1) {
  Engine e;
  e.prepare(48000, 256, 1);
  e.setParameters(p);
  std::vector<float> out;
  out.reserve(seconds * 48000);
  std::array<float, 256> a{}, sc{};
  float *ptr[] = {a.data()};
  const float *sp[] = {sc.data()};
  for (int pos = 0; pos < seconds * 48000; pos += 256) {
    if (impulse >= 0) {
      p[index("throw.trigger")] =
          (pos <= impulse && pos + 256 > impulse) ? 1.f : 0.f;
      e.setParameters(p);
    }
    for (int i = 0; i < 256; ++i) {
      int t = pos + i;
      a[i] = impulse >= 0 ? (t == impulse ? .5f : 0)
                          : .1f * std::sin(2 * malle::pi * hz * t / 48000);
      sc[i] = .3f;
    }
    e.process(ptr, 1, 256, side ? sp : nullptr, side ? 1 : 0,
              {120, pos / 24000., true, true});
    out.insert(out.end(), a.begin(), a.end());
  }
  return out;
}
float rms(const std::vector<float> &v) {
  double s = 0;
  size_t start = v.size() / 2;
  for (size_t i = start; i < v.size(); ++i)
    s += v[i] * v[i];
  return float(std::sqrt(s / (v.size() - start)));
}
float frequency(const std::vector<float> &v) {
  int count = 0;
  size_t start = v.size() / 2;
  for (size_t i = start + 1; i < v.size(); ++i)
    if (v[i - 1] <= 0 && v[i] > 0)
      ++count;
  return count * 48000.f / (v.size() - start);
}
int main() {
  try {
    auto p = neutral();
    p[index("tune.on")] = 1;
    p[index("pitch")] = 12;
    p[index("tune.amount")] = 0;
    auto shifted = render(p, 220);
    float f = frequency(shifted);
    require(std::abs(f - 440) < 2, "octave pitch shift");
    p[index("pitch")] = 0;
    p[index("tune.amount")] = 1;
    p[index("tune.speed")] = 0;
    p[index("tune.human")] = 0;
    f = frequency(render(p, 224));
    require(std::abs(f - 220) < 2, "monophonic hard tune converges");
    std::cout << "Tuning 224 Hz -> " << f << " Hz\n";
    p = neutral();
    float tone = 120 * std::pow(150.f, 15 / 31.f);
    float ref = rms(render(p, tone));
    p[index("res.on")] = 1;
    p[index("res.amount")] = 1;
    p[index("res.max")] = 12;
    p[index("res.select")] = 1;
    float reduced = rms(render(p, tone));
    std::cout << "Resonance " << tone << " Hz: " << db(reduced / ref)
              << " dB\n";
    require(reduced < ref * .99f, "detected resonance attenuation");
    p = neutral();
    ref = rms(render(p, 10000));
    p[index("air.on")] = 1;
    p[index("air.high")] = 1;
    p[index("air.guard")] = 0;
    float airy = rms(render(p, 10000));
    require(airy > ref * 1.2f, "air raises high frequency");
    std::cout << "Air 10 kHz: +" << db(airy / ref) << " dB\n";
    p = neutral();
    p[index("pump.on")] = 1;
    p[index("pump.mode")] = 1;
    p[index("pump.target")] = 0;
    p[index("pump.depth")] = 1;
    float pumped = rms(render(p, 220, 3, true));
    require(pumped < .04f, "external sidechain compression");
    std::cout << "External SC output RMS: " << pumped << "\n";
    p = neutral();
    p[index("throw.on")] = 1;
    p[index("throw.send")] = 1;
    p[index("throw.sync")] = 0;
    p[index("throw.time")] = 10;
    p[index("throw.feedback")] = .5;
    p[index("throw.width")] = 0;
    p[index("throw.window")] = 10;
    int imp = 256 * 80;
    auto thrown = render(p, 0, 2, false, imp);
    Engine e;
    e.prepare(48000, 256, 1);
    int expected = imp + e.latency() + 480;
    float early = 0, late = 0;
    for (int i = expected - 300; i < expected - 10; ++i)
      early += std::abs(thrown[size_t(i)]);
    for (int i = expected; i < expected + 1500; ++i)
      late += std::abs(thrown[size_t(i)]);
    require(early < 1e-6f && late > .1f,
            "latency-aligned throw and continuing tail");
    std::cout << "Throw delay: correct latency/window, tail energy " << late
              << "\nPASS: measured DSP behaviour\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
