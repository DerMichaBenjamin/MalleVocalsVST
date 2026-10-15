#pragma once
#include <array>
#include <string>
#include <string_view>
#include <vector>
namespace malle {
struct Param {
  std::string id, name, section, unit;
  float min, max, def;
  bool logarithmic = false;
};
const std::vector<Param> &parameters();
int index(std::string_view id);
bool discrete(std::string_view id);
using Values = std::vector<float>;
Values defaults();
struct VariantProfile {
  std::string name, role, purpose;
  float compression, ratio, attack, release, body, presence, air, drive, warmth,
      width, time, voices, stereo, space, feedback, pump, shape;
};
const std::array<VariantProfile, 20> &profiles();
struct Preset {
  std::string name, category;
  Values values;
  std::string description, reference, status = "Technical starting point";
};
const std::vector<Preset> &presets();
const std::array<std::string, 20> &variants();
// Macro values are offsets from factory-neutral bases. Detail automation
// changes the base; moving a macro preserves that offset. No UI mode is part of
// DSP state.
Values resolve(const Values &base);
void resolveInto(const Values &base, Values &result);
} // namespace malle
