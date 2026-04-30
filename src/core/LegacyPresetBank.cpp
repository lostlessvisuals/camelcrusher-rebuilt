#include "camelcrusher_recalled/LegacyPresetBank.h"

namespace camelcrusher_recalled {

LegacyPresetBank legacyPresetBankFromChunk(const LegacyChunk& chunk) {
  LegacyPresetBank preset_bank;
  preset_bank.presets.reserve(chunk.programs.size());

  for (std::size_t index = 0; index < chunk.programs.size(); ++index) {
    const auto& program = chunk.programs[index];
    preset_bank.presets.push_back(LegacyPreset{
        .program_index = index,
        .record_marker = program.record_marker,
        .name = program.name,
        .state = legacyStateFromArray(program.parameter_values),
    });
  }

  return preset_bank;
}

const LegacyPreset* legacyPresetAt(const LegacyPresetBank& preset_bank,
                                   const std::size_t program_index) {
  if (program_index >= preset_bank.presets.size()) {
    return nullptr;
  }

  return &preset_bank.presets[program_index];
}

std::optional<std::size_t> legacyPresetIndexByName(
    const LegacyPresetBank& preset_bank,
    const std::string_view name) {
  for (std::size_t index = 0; index < preset_bank.presets.size(); ++index) {
    if (preset_bank.presets[index].name == name) {
      return index;
    }
  }

  return std::nullopt;
}

bool hasExpectedLegacyProgramCount(const LegacyPresetBank& preset_bank) {
  return preset_bank.presets.size() == kLegacyProgramCount;
}

}  // namespace camelcrusher_recalled
