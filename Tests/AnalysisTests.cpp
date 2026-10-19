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
void checkThrowStereo() {
  auto p = neutral();
  p[index("throw.on")] = 1; p[index("throw.send")] = 1;
  p[index("throw.return")] = 1; p[index("throw.sync")] = 0;
  p[index("throw.time")] = 100; p[index("throw.feedback")] = .7f;
  p[index("throw.type")] = 2; p[index("throw.window")] = 1000;
  p[index("throw.low")] = 20; p[index("throw.high")] = 18000;
  Engine e; e.prepare(48000,256,2);
  std::array<float,256> l{}, r{}; float* channels[]{l.data(),r.data()};
  std::vector<float> a,b;
  for (int t=0;t<48000;t+=256) {
    p[index("throw.trigger")] = t==0 ? 1.f : 0.f;
    e.setParameters(p);
    for(int i=0;i<256;++i) l[i]=r[i]=(t+i==0 || t+i==2400) ? .5f : 0.f;
    e.process(channels,2,256,nullptr,0,{120,t/24000.,true,true});
    a.insert(a.end(),l.begin(),l.end()); b.insert(b.end(),r.begin(),r.end());
  }
  auto energy = [](const std::vector<float>& v,int at) {
    double sum=0; for(int i=at;i<at+500;++i)sum+=v[size_t(i)]*v[size_t(i)]; return sum;
  };
  int first=e.latency()+4800, second=first+4800;
  require(energy(a,first)>energy(b,first)*100, "ping pong first echo left");
  require(energy(b,second)>energy(a,second)*100, "ping pong second echo right");
  require(energy(a,first+2400)+energy(b,first+2400)<1e-6,
          "release closes aligned send before second syllable");
  p[index("throw.kill")]=1; e.setParameters(p);
  double tail=0;
  for(int t=0;t<12000;t+=256) {
    l.fill(0);r.fill(0);e.process(channels,2,256,nullptr,0,{});
    for(int i=0;i<256;++i)tail+=l[i]*l[i]+r[i]*r[i];
  }
  require(tail<1e-6,"throw tail kill");
  std::cout << "Ping pong/release/tail kill: PASS\n";
}
int main() {
  try {
    checkThrowStereo();
    auto p = neutral();
    p[index("tune.on")] = 1;
    p[index("pitch")] = 12;
    p[index("creative.on")] = 1;
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
    // Capture after the 3-ms send fade has opened, rather than exactly at its
    // first sample (which is deliberately attenuated to prevent clicks).
    int imp = 256 * 80 + 192;
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
    auto routeBefore = neutral();
    routeBefore[index("fet.on")] = 1;
    routeBefore[index("fet.threshold")] = -26;
    routeBefore[index("fet.ratio")] = 8;
    routeBefore[index("eq.on")] = 1;
    routeBefore[index("eq0.freq")] = 220;
    routeBefore[index("eq0.gain")] = 12;
    routeBefore[index("eq0.q")] = 2;
    float a = rms(render(routeBefore, 220));
    routeBefore[index("route.2")] = 5;
    routeBefore[index("route.5")] = 2;
    float b = rms(render(routeBefore, 220));
    require(std::abs(db(a / b)) > 1, "module order changes audio");
    std::cout << "Routing EQ/FET order difference: " << db(a / b) << " dB\n";
    std::cout << "Throw delay: correct latency/window, tail energy " << late
              << "\nPASS: measured DSP behaviour\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
