#include "dsp/Engine.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace malle;
uint32_t read32(std::ifstream &f) {
  uint32_t v;
  f.read(reinterpret_cast<char *>(&v), 4);
  return v;
}
int main(int argc, char **argv) {
  if (argc < 4) {
    std::cerr << "input-float.wav preset-index output-float.wav [input-gain-db]\n";
    return 1;
  }
  std::ifstream f(argv[1], std::ios::binary);
  char tag[4];
  f.read(tag, 4);
  if (std::memcmp(tag, "RIFF", 4))
    return 2;
  read32(f);
  f.read(tag, 4);
  int channels = 0, rate = 0, bits = 0, format = 0;
  std::vector<char> bytes;
  while (f) {
    f.read(tag, 4);
    uint32_t size = read32(f);
    if (!f)
      break;
    if (!std::memcmp(tag, "fmt ", 4)) {
      std::vector<char> fmt(size);
      f.read(fmt.data(), size);
      format = *reinterpret_cast<uint16_t *>(fmt.data());
      channels = *reinterpret_cast<uint16_t *>(fmt.data() + 2);
      rate = *reinterpret_cast<uint32_t *>(fmt.data() + 4);
      bits = *reinterpret_cast<uint16_t *>(fmt.data() + 14);
    } else if (!std::memcmp(tag, "data", 4)) {
      bytes.resize(size);
      f.read(bytes.data(), size);
    } else
      f.seekg(size, std::ios::cur);
    if (size % 2)
      f.seekg(1, std::ios::cur);
  }
  int preset = std::stoi(argv[2]);
  if (format != 3 || bits != 32 || channels < 1 || channels > 2 || preset < 0 ||
      preset >= int(presets().size()))
    return 3;
  size_t frames = bytes.size() / (4 * channels);
  std::vector<float> audio(frames * size_t(channels));
  std::memcpy(audio.data(), bytes.data(), bytes.size());
  double energy = 0;
  for (float x : audio)
    energy += x * x;
  float calibration =
      .0630957f /
      float(std::sqrt(energy / std::max(size_t(1), audio.size())) + 1e-9);
  calibration = std::min(calibration, 20.f);
  if (argc > 4) calibration *= gain(std::stof(argv[4]));
  Engine e;
  e.prepare(rate, 256, channels);
  auto params = resolve(presets()[size_t(preset)].values);
  e.setParameters(params);
  std::array<std::array<float, 256>, 2> b{};
  float *ptr[] = {b[0].data(), b[1].data()};
  std::vector<float> out;
  size_t total = frames + size_t(e.latency()) + rate * 2;
  out.reserve((total + 256) * channels);
  for (size_t pos = 0; pos < total; pos += 256) {
    for (int c = 0; c < channels; ++c)
      for (int i = 0; i < 256; ++i)
        b[c][i] = pos + i < frames
                      ? audio[(pos + i) * channels + c] * calibration
                      : 0;
    e.process(ptr, channels, 256, nullptr, 0,
              {120, pos / float(rate) * 2, true, true});
    for (int i = 0; i < 256; ++i)
      for (int c = 0; c < channels; ++c)
        out.push_back(b[c][i]);
  }
  std::ofstream w(argv[3], std::ios::binary);
  auto u32 = [&](uint32_t v) { w.write(reinterpret_cast<char *>(&v), 4); };
  auto u16 = [&](uint16_t v) { w.write(reinterpret_cast<char *>(&v), 2); };
  w.write("RIFF", 4);
  u32(uint32_t(36 + out.size() * 4));
  w.write("WAVEfmt ", 8);
  u32(16);
  u16(3);
  u16(uint16_t(channels));
  u32(uint32_t(rate));
  u32(uint32_t(rate * channels * 4));
  u16(uint16_t(channels * 4));
  u16(32);
  w.write("data", 4);
  u32(uint32_t(out.size() * 4));
  w.write(reinterpret_cast<char *>(out.data()),
          std::streamsize(out.size() * 4));
  std::cout << e.latency() << '\n';
  return w ? 0 : 4;
}
