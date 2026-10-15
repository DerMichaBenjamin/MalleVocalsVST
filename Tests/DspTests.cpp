#include "dsp/Engine.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
static thread_local bool countAllocations = false;
static thread_local size_t allocationCount = 0;
void *operator new(std::size_t n) {
  if (countAllocations)
    ++allocationCount;
  if (auto *p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }
#include <stdexcept>
using namespace malle;
void check(bool b, const char *msg) {
  if (!b)
    throw std::runtime_error(msg);
}
int main() {
  try {
    check(presets().size() == 55, "55 global presets");
    check(variants().size() == 20, "20 shared variants");
    for (auto &pr : presets()) {
      auto v = resolve(pr.values);
      check(v.size() == parameters().size(), "preset width");
      for (size_t i = 0; i < v.size(); ++i)
        check(v[i] >= parameters()[i].min && v[i] <= parameters()[i].max,
              "preset range");
    }
    for (int rate : {44100, 48000, 96000})
      for (int nc : {1, 2})
        for (int block : {1, 64, 257, 1024}) {
          Engine e;
          e.prepare(rate, block, nc);
          auto p = defaults();
          p[index("bypass")] = 1;
          e.setParameters(resolve(p));
          std::array<std::vector<float>, 2> a;
          for (auto &v : a)
            v.resize(block);
          float *ptr[] = {a[0].data(), a[1].data()};
          int total = e.latency() + block * 3;
          float maxError = 0;
          for (int pos = 0; pos < total; pos += block) {
            for (int c = 0; c < nc; ++c)
              for (int i = 0; i < block; ++i)
                a[c][i] = .1f * std::sin(2 * malle::pi * 220 * (pos + i) / rate);
            e.process(ptr, nc, block, nullptr, 0, {});
            for (int c = 0; c < nc; ++c)
              for (int i = 0; i < block; ++i) {
                int t = pos + i - e.latency();
                float expected =
                    t < 0 ? 0 : .1f * std::sin(2 * malle::pi * 220 * t / rate);
                maxError = std::max(maxError, std::abs(a[c][i] - expected));
              }
          }
          check(maxError < 1e-6f, "latency aligned bypass");
        }
    Engine e;
    e.prepare(48000, 256, 2);
    std::array<std::array<float, 256>, 2> a{};
    float *ptr[] = {a[0].data(), a[1].data()};
    for (auto &pr : presets()) {
      auto p = resolve(pr.values);
      e.reset();
      e.setParameters(p);
      for (int n = 0; n < 80; ++n) {
        for (int c = 0; c < 2; ++c)
          for (int i = 0; i < 256; ++i)
            a[c][i] = .15f * std::sin(2 * malle::pi * 220 * (n * 256 + i) / 48000);
        e.process(ptr, 2, 256, nullptr, 0,
                  {128, n * 256 * 128. / (48000 * 60), true, true});
        for (auto &v : a)
          for (float x : v)
            check(std::isfinite(x) && std::abs(x) < 20,
                  "finite bounded output");
      }
    }
    check(std::abs(e.meters.pitch.load() - 57) < .25, "YIN 220Hz pitch");
    auto base = defaults(), resolved = base;
    e.reset();
    for (auto &v : a)
      v.fill(.1f);
    allocationCount = 0;
    countAllocations = true;
    resolveInto(base, resolved);
    e.setParameters(resolved);
    e.process(ptr, 2, 256, nullptr, 0, {});
    countAllocations = false;
    check(allocationCount == 0, "audio callback must not allocate");
    // All module switches remain authoritative even when macros are active.
    base[index("waerme.amount")] = 1;
    base[index("sat.on")] = 0;
    resolved = resolve(base);
    check(resolved[index("sat.on")] == 0, "detail bypass overrides macro");
    std::cout << "PASS: 55 presets, 20 variants, 24 rate/channel/buffer "
                 "configurations, exact latency/bypass, finite outputs, pitch "
                 "detection\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
