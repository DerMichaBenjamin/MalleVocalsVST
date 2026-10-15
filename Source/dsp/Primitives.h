#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>
namespace malle {
constexpr float pi = 3.14159265358979323846f;
inline float gain(float db) { return std::pow(10.f, db * .05f); }
inline float db(float a) { return 20 * std::log10(std::max(a, 1e-9f)); }
inline float coef(float ms, float sr) {
  return std::exp(-1.f / (std::max(.01f, ms) * .001f * sr));
}
struct Biquad {
  float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  float tick(float x) {
    float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
  }
  void reset() { z1 = z2 = 0; }
  void set(int type, float hz, float q, float g, float sr) {
    hz = std::clamp(hz, 10.f, sr * .46f);
    q = std::max(.2f, q);
    float w = 2 * pi * hz / sr, c = std::cos(w), s = std::sin(w),
          al = s / (2 * q), A = std::sqrt(gain(g)), a0 = 1 + al;
    float B0 = 1, B1 = 0, B2 = 0, A1 = -2 * c, A2 = 1 - al;
    if (type == 0) {
      B0 = (1 - c) / 2;
      B1 = 1 - c;
      B2 = B0;
    } else if (type == 1) {
      B0 = (1 + c) / 2;
      B1 = -1 - c;
      B2 = B0;
    } else if (type == 2) {
      B0 = al;
      B1 = 0;
      B2 = -al;
    } else if (type == 3) {
      B0 = 1 + al * A;
      B1 = -2 * c;
      B2 = 1 - al * A;
      a0 = 1 + al / A;
      A2 = 1 - al / A;
    } else {
      float t = 2 * std::sqrt(A) * al;
      if (type == 4) {
        B0 = A * ((A + 1) - (A - 1) * c + t);
        B1 = 2 * A * ((A - 1) - (A + 1) * c);
        B2 = A * ((A + 1) - (A - 1) * c - t);
        a0 = (A + 1) + (A - 1) * c + t;
        A1 = -2 * ((A - 1) + (A + 1) * c);
        A2 = (A + 1) + (A - 1) * c - t;
      } else {
        B0 = A * ((A + 1) + (A - 1) * c + t);
        B1 = -2 * A * ((A - 1) + (A + 1) * c);
        B2 = A * ((A + 1) + (A - 1) * c - t);
        a0 = (A + 1) - (A - 1) * c + t;
        A1 = 2 * ((A - 1) - (A + 1) * c);
        A2 = (A + 1) - (A - 1) * c - t;
      }
    }
    b0 = B0 / a0;
    b1 = B1 / a0;
    b2 = B2 / a0;
    a1 = A1 / a0;
    a2 = A2 / a0;
  }
};
struct Envelope {
  float e = 0;
  float tick(float x, float a, float r) {
    x = std::abs(x);
    e = (x > e ? a : r) * (e - x) + x;
    return e;
  }
};
struct Compressor {
  Envelope detector;
  float reduction = 0;
  float tickGain(float x, float threshold, float ratio, float attack,
                 float release, float knee, float sr) {
    float level = db(detector.tick(x, coef(attack, sr), coef(release, sr))),
          over = level - threshold, red = 0;
    float slope = 1 - 1 / ratio;
    if (knee > 0 && over > -knee / 2 && over < knee / 2)
      red = slope * (over + knee / 2) * (over + knee / 2) / (2 * knee);
    else if (over > 0)
      red = slope * over;
    reduction = red;
    return gain(-red);
  }
};
struct Delay {
  std::vector<float> data;
  int pos = 0;
  void prepare(int n) {
    data.assign(n, 0);
    pos = 0;
  }
  float read(float samples) const {
    float p = float(pos) - std::clamp(samples, 1.f, float(data.size() - 2));
    if (p < 0)
      p += float(data.size());
    int i = int(p);
    float f = p - i;
    return data[size_t(i)] * (1 - f) +
           data[size_t((i + 1) % int(data.size()))] * f;
  }
  void push(float x) {
    data[size_t(pos)] = x;
    if (++pos == int(data.size()))
      pos = 0;
  }
};
// Eight-line orthogonal feedback delay network. Normalised Hadamard mixing
// keeps feedback energy bounded independently of room size and sample rate.
struct Reverb {
 std::array<Delay,8> line;std::array<float,8> damping{},tap{},feedback{};float sr=48000,alpha=.5f;
 void prepare(float rate){sr=rate;for(auto&d:line)d.prepare(int(sr*.25f));damping.fill(0);configure(.5f,2,6500,.7f);}
 void reset(){for(auto&d:line){std::fill(d.data.begin(),d.data.end(),0);d.pos=0;}damping.fill(0);}
 void configure(float size,float decay,float high,float character){constexpr std::array<float,8> times{.0297f,.0371f,.0411f,.0437f,.0531f,.0617f,.0713f,.0799f};float scale=.45f+size*1.5f;alpha=1-std::exp(-2*pi*std::clamp(high,500.f,sr*.4f)/sr);for(int j=0;j<8;++j){tap[j]=times[j]*sr*scale;feedback[j]=std::pow(.001f,times[j]*scale/std::max(.1f,decay))*(.75f+.24f*character);}}
 float tick(float input){std::array<float,8>v{};for(int j=0;j<8;++j){float x=line[j].read(tap[j]);damping[j]+=alpha*(x-damping[j]);v[j]=damping[j];}float out=(v[0]-v[1]+v[2]+v[3]-v[4]+v[5]-v[6]+v[7])*.25f;for(int step=1;step<8;step*=2)for(int j=0;j<8;j+=step*2)for(int k=0;k<step;++k){float a=v[j+k],b=v[j+k+step];v[j+k]=a+b;v[j+k+step]=a-b;}for(int j=0;j<8;++j)line[j].push(input*(j%2?.3f:-.3f)+v[j]*.35355339f*feedback[j]);return out;}
};
struct Sample {
  std::vector<float> left, right;
  double sampleRate = 48000;
  std::string name = "Synthetic test crowd (original)";
};
} // namespace malle
