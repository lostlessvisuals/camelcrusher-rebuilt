#include "camelcrusher_recalled/LegacySchema.h"

namespace camelcrusher_recalled {

std::optional<std::size_t> legacyParameterIndexByName(std::string_view name) {
  for (std::size_t index = 0; index < kLegacyParameterSpecs.size(); ++index) {
    if (kLegacyParameterSpecs[index].name == name) {
      return index;
    }
  }

  return std::nullopt;
}

bool validateLegacyParameterOrdering(std::span<const std::string_view> names) {
  if (names.size() != kLegacyParameterSpecs.size()) {
    return false;
  }

  for (std::size_t index = 0; index < names.size(); ++index) {
    if (names[index] != kLegacyParameterSpecs[index].name) {
      return false;
    }
  }

  return true;
}

}  // namespace camelcrusher_recalled
