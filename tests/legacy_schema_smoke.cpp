#include "camelcrusher_recalled/LegacySchema.h"

#include <array>
#include <cassert>
#include <iostream>

int main() {
  using camelcrusher_recalled::kLegacyParameterCount;
  using camelcrusher_recalled::kLegacyParameterSpecs;
  using camelcrusher_recalled::kLegacyUniqueId;
  using camelcrusher_recalled::kLegacyUniqueIdFourCC;
  using camelcrusher_recalled::legacyParameterIndexByName;
  using camelcrusher_recalled::validateLegacyParameterOrdering;

  assert(kLegacyUniqueId == 1130447730);
  assert(kLegacyUniqueIdFourCC == "CaCr");

  std::array<std::string_view, kLegacyParameterCount> parameter_names{};
  for (std::size_t index = 0; index < parameter_names.size(); ++index) {
    parameter_names[index] = kLegacyParameterSpecs[index].name;
  }

  assert(validateLegacyParameterOrdering(parameter_names));
  assert(legacyParameterIndexByName("MasterVolume").has_value());
  assert(*legacyParameterIndexByName("MasterVolume") == 11);
  assert(!legacyParameterIndexByName("NotARealParameter").has_value());

  std::cout << "Legacy schema smoke test passed\n";
  return 0;
}
