#pragma once
#include <string>
namespace malle {
constexpr int crowdSlots = 7;
inline std::string crowdProperty(int slot, const char* field) {
  return "crowd." + std::to_string(slot) + "." + field;
}
// Slot 1 retains all original automation IDs.
inline std::string crowdParameter(int slot, const char* field) {
  return slot == 0 ? "stadium." + std::string(field)
                   : "stadium." + std::to_string(slot) + "." + field;
}
}
