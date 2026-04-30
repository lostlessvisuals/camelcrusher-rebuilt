#pragma once

#include "camelcrusher_recalled/LegacyPresetBank.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace camelcrusher_recalled {

enum class LegacyCurrentStateSource {
  kNone,
  kSelectedProgram,
  kExplicitParameters,
};

struct LegacyParameterMismatch {
  LegacyParameterId id = LegacyParameterId::kDistOn;
  std::size_t legacy_index = 0;
  float selected_program_value = 0.0F;
  float explicit_parameter_value = 0.0F;
};

struct LegacyImportedState {
  LegacyCurrentStateSource current_state_source = LegacyCurrentStateSource::kNone;
  LegacyState current_state;
  LegacyPresetBank preset_bank;
  std::optional<std::size_t> selected_program_index;
  std::string selected_program_name;
  bool selected_program_matches_explicit_parameters = false;
  std::vector<LegacyParameterMismatch> parameter_mismatches;
};

LegacyImportedState importLegacyState(
    const LegacyChunk& chunk,
    std::optional<int> program_number,
    std::span<const float> explicit_parameter_values = {});
LegacyParameterArray importedStateToArray(const LegacyImportedState& imported_state);

}  // namespace camelcrusher_recalled
