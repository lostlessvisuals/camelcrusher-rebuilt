#include "camelcrusher_recalled/LegacyImport.h"

#include <cmath>

namespace camelcrusher_recalled {
namespace {

bool nearlyEqual(const float left, const float right) {
  return std::abs(left - right) < 0.000001F;
}

}  // namespace

LegacyImportedState importLegacyState(
    const LegacyChunk& chunk,
    const std::optional<int> program_number,
    const std::span<const float> explicit_parameter_values) {
  LegacyImportedState imported_state;
  imported_state.preset_bank = legacyPresetBankFromChunk(chunk);

  if (program_number.has_value() && *program_number >= 0 &&
      static_cast<std::size_t>(*program_number) <
          imported_state.preset_bank.presets.size()) {
    const auto program_index = static_cast<std::size_t>(*program_number);
    const auto& program = imported_state.preset_bank.presets[program_index];
    imported_state.selected_program_index = program_index;
    imported_state.selected_program_name = program.name;
    imported_state.current_state = program.state;
    imported_state.current_state_source = LegacyCurrentStateSource::kSelectedProgram;
  }

  if (explicit_parameter_values.size() == kLegacyParameterCount) {
    imported_state.current_state = legacyStateFromArray(explicit_parameter_values);
    imported_state.current_state_source =
        LegacyCurrentStateSource::kExplicitParameters;

    if (imported_state.selected_program_index.has_value()) {
      const auto* selected_program = legacyPresetAt(
          imported_state.preset_bank, imported_state.selected_program_index.value());

      if (selected_program != nullptr) {
        const auto selected_program_values = legacyStateToArray(selected_program->state);

        for (const auto& descriptor : legacyParameterDescriptors()) {
          if (!nearlyEqual(selected_program_values[descriptor.legacy_index],
                           explicit_parameter_values[descriptor.legacy_index])) {
            imported_state.parameter_mismatches.push_back(LegacyParameterMismatch{
                .id = descriptor.id,
                .legacy_index = descriptor.legacy_index,
                .selected_program_value =
                    selected_program_values[descriptor.legacy_index],
                .explicit_parameter_value =
                    explicit_parameter_values[descriptor.legacy_index],
            });
          }
        }

        imported_state.selected_program_matches_explicit_parameters =
            imported_state.parameter_mismatches.empty();
      }
    }
  }

  return imported_state;
}

LegacyParameterArray importedStateToArray(
    const LegacyImportedState& imported_state) {
  return legacyStateToArray(imported_state.current_state);
}

}  // namespace camelcrusher_recalled
