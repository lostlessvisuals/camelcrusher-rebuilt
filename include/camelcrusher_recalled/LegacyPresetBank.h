#pragma once

#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyState.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace camelcrusher_recalled {

struct LegacyPreset {
  std::size_t program_index = 0;
  float record_marker = 1.0F;
  std::string name;
  LegacyState state;
};

struct LegacyPresetBank {
  std::vector<LegacyPreset> presets;
};

LegacyPresetBank legacyPresetBankFromChunk(const LegacyChunk& chunk);
const LegacyPreset* legacyPresetAt(const LegacyPresetBank& preset_bank,
                                   std::size_t program_index);
std::optional<std::size_t> legacyPresetIndexByName(
    const LegacyPresetBank& preset_bank,
    std::string_view name);
bool hasExpectedLegacyProgramCount(const LegacyPresetBank& preset_bank);

}  // namespace camelcrusher_recalled
