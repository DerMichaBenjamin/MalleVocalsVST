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
// Slow, bounded RMS comparison. Silence freezes the estimate; no look-ahead,
// allocations or attempts to cancel deliberate rhythmic attenuation.
struct LevelMatch {
  float input=0,output=0,correction=1;
  float tick(float in,float out,float sr,bool enabled) {
    if(!enabled)return 1;
    float a=coef(350,sr);
    input=a*input+(1-a)*in; output=a*output+(1-a)*out;
    if(input>1e-6f && output>1e-9f) {
      float target=std::clamp(std::sqrt(input/output),.2511886f,7.943282f);
      float b=coef(200,sr);correction=b*correction+(1-b)*target;
    }
    return correction;
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
  std::array<Delay, 8> line;
  std::array<Delay,4> diffuser;
  bool diffuse=false;
  std::array<float, 8> damping{}, tap{}, feedback{};
  float sr = 48000, alpha = .5f;
  void prepare(float rate) {
    sr = rate;
    for (auto &d : line)
      d.prepare(int(sr * .25f));
    for(auto& d:diffuser)d.prepare(int(sr*.04f)+8);
    damping.fill(0);
    configure(.5f, 2, 6500, .7f);
  }
  void reset() {
    for (auto &d : line) {
      std::fill(d.data.begin(), d.data.end(), 0);
      d.pos = 0;
    }
    for(auto& d:diffuser){std::fill(d.data.begin(),d.data.end(),0);d.pos=0;}
    damping.fill(0);
  }
  void configure(float size, float decay, float high, float character, bool dense=false) {
    diffuse=dense;
    constexpr std::array<float, 8> times{.0297f, .0371f, .0411f, .0437f,
                                         .0531f, .0617f, .0713f, .0799f};
    float scale = .45f + size * 1.5f;
    alpha = 1 - std::exp(-2 * pi * std::clamp(high, 500.f, sr * .4f) / sr);
    for (int j = 0; j < 8; ++j) {
      tap[j] = times[j] * sr * scale;
      feedback[j] = std::pow(.001f, times[j] * scale / std::max(.1f, decay)) *
                    (dense ? 1.f : (.75f + .24f * character));
    }
  }
  float tick(float input) {
    if(diffuse) {
      constexpr std::array<float,4> seconds{.0047f,.0083f,.0127f,.0193f};
      for(size_t i=0;i<diffuser.size();++i){
        float delayed=diffuser[i].read(seconds[i]*sr);
        float y=delayed-.65f*input;
        diffuser[i].push(input+.65f*y); input=y;
      }
    }
    std::array<float, 8> v{};
    for (int j = 0; j < 8; ++j) {
      float x = line[j].read(tap[j]);
      damping[j] += alpha * (x - damping[j]);
      v[j] = damping[j];
    }
    float out = (v[0] - v[1] + v[2] + v[3] - v[4] + v[5] - v[6] + v[7]) * .25f;
    for (int step = 1; step < 8; step *= 2)
      for (int j = 0; j < 8; j += step * 2)
        for (int k = 0; k < step; ++k) {
          float a = v[j + k], b = v[j + k + step];
          v[j + k] = a + b;
          v[j + k + step] = a - b;
        }
    for (int j = 0; j < 8; ++j)
      line[j].push(input * (j % 2 ? .3f : -.3f) +
                   v[j] * .35355339f * feedback[j]);
    return out*(diffuse?2.5f:1.f);
  }
};
struct Sample {
  std::vector<float> left, right;
  double sampleRate = 48000;
  std::string name = "Synthetic test crowd (original)";
};
// Immutable sample readers; no ownership changes or allocation on the audio thread.
struct CrowdPlayer {
  const Sample *active = nullptr, *previous = nullptr;
  double pos = 0, previousPos = 0;
  float blend = 1;
  void reset() { active=previous=nullptr; pos=previousPos=0; blend=1; }
  static std::array<float,2> read(const Sample* s, double& at, bool loop,
                                 float fadeMs, bool smooth, double step) {
    if (!s || s->left.empty()) return {};
    const double len=double(s->left.size());
    const double fade=smooth ? std::clamp(double(fadeMs)*.001*s->sampleRate,1.,std::max(1.,len*.25)) : std::max(1.,double(fadeMs)*.001*s->sampleRate);
    if(loop && at>=len) at=smooth ? fade+std::fmod(at-len,len-fade) : std::fmod(at,len);
    if(at>=len) return {};
    auto sampleAt=[&](const std::vector<float>& v,double t) {
      if(v.empty()) return 0.f;
      size_t a=std::min(size_t(t),v.size()-1),b=std::min(a+1,v.size()-1);
      float f=float(t-double(a));return v[a]+(v[b]-v[a])*f;
    };
    std::array<float,2> y{};
    for(int c=0;c<2;++c) {
      const auto& v=(c==0 || s->right.empty()) ? s->left : s->right;
      float x=sampleAt(v,at);
      if(loop && smooth && at>=len-fade) {
        const float t=float((at-(len-fade))/fade);
        x=x*(1-t)+sampleAt(v,at-(len-fade))*t;
      } else {
        const double edge=(loop && smooth) ? at : std::min(at,len-1-at);
        x*=float(std::clamp(edge/fade,0.,1.));
      }
      y[size_t(c)]=x;
    }
    at+=step;return y;
  }
  std::array<float,2> next(const Sample* s, float sr, float speed, bool loop,
                          float fadeMs, bool smooth, bool restart) {
    if(active!=s) {
      previous=active; previousPos=pos; active=s; pos=0;
      blend=previous ? 0.f : 1.f;
    }
    if(restart)pos=0;
    auto y=read(active,pos,loop,fadeMs,smooth,s ? s->sampleRate/sr*speed : 0);
    if(blend<1) {
      auto old=read(previous,previousPos,loop,fadeMs,smooth,
                    previous ? previous->sampleRate/sr*speed : 0);
      blend=std::min(1.f,blend+1.f/(sr*.01f));
      for(int c=0;c<2;++c)y[size_t(c)]=old[size_t(c)]*(1-blend)+y[size_t(c)]*blend;
    }
    return y;
  }
};
} // namespace malle
