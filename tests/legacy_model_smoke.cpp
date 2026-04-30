#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyImport.h"
#include "camelcrusher_recalled/LegacyParameters.h"
#include "camelcrusher_recalled/LegacyPresetBank.h"
#include "camelcrusher_recalled/LegacySchema.h"
#include "camelcrusher_recalled/LegacyState.h"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>

namespace {

using camelcrusher_recalled::decodeLegacyChunk;
using camelcrusher_recalled::encodeLegacyChunk;
using camelcrusher_recalled::hasExpectedLegacyProgramCount;
using camelcrusher_recalled::importedStateToArray;
using camelcrusher_recalled::importLegacyState;
using camelcrusher_recalled::isReservedCompatibilityParameter;
using camelcrusher_recalled::isSwitchLikeParameter;
using camelcrusher_recalled::legacyParameterDescriptor;
using camelcrusher_recalled::legacyParameterDescriptors;
using camelcrusher_recalled::legacyParameterIdFromName;
using camelcrusher_recalled::legacyPresetAt;
using camelcrusher_recalled::legacyPresetBankFromChunk;
using camelcrusher_recalled::legacyPresetIndexByName;
using camelcrusher_recalled::legacyStateFromArray;
using camelcrusher_recalled::legacyStateMatchesArray;
using camelcrusher_recalled::legacyStateToArray;
using camelcrusher_recalled::LegacyChunk;
using camelcrusher_recalled::LegacyCurrentStateSource;
using camelcrusher_recalled::LegacyParameterArray;
using camelcrusher_recalled::LegacyParameterId;
using camelcrusher_recalled::LegacyProgram;
using camelcrusher_recalled::kLegacyParameterSpecs;
using camelcrusher_recalled::kLegacyParameterCount;

LegacyProgram makeProgram(const std::string& name, const float seed) {
  LegacyProgram program;
  program.record_marker = 1.0F;
  program.name = name;
  for (std::size_t index = 0; index < program.parameter_values.size(); ++index) {
    program.parameter_values[index] =
        seed + static_cast<float>(index) * 0.03125F;
  }
  return program;
}

bool nearlyEqual(const float left, const float right) {
  return std::fabs(left - right) < 0.000001F;
}

}  // namespace

int main() {
  const auto descriptors = legacyParameterDescriptors();
  assert(descriptors.size() == kLegacyParameterCount);
  assert(descriptors[0].id == LegacyParameterId::kDistOn);
  assert(descriptors[0].legacy_index == 0U);
  assert(legacyParameterDescriptor(LegacyParameterId::kMasterVolume).legacy_index ==
         11U);
  for (std::size_t index = 0; index < descriptors.size(); ++index) {
    assert(descriptors[index].legacy_index == index);
    assert(descriptors[index].key == kLegacyParameterSpecs[index].name);
  }
  assert(legacyParameterIdFromName("CompressAmount").has_value());
  assert(legacyParameterIdFromName("CompressAmount").value() ==
         LegacyParameterId::kCompressAmount);
  assert(isSwitchLikeParameter(LegacyParameterId::kDistOn));
  assert(!isSwitchLikeParameter(LegacyParameterId::kMasterMix));
  assert(isReservedCompatibilityParameter(LegacyParameterId::kUnused5));

  LegacyParameterArray array_values{};
  for (std::size_t index = 0; index < array_values.size(); ++index) {
    array_values[index] = static_cast<float>(index) / 16.0F;
  }

  const auto state = legacyStateFromArray(array_values);
  const auto round_tripped_values = legacyStateToArray(state);
  assert(legacyStateMatchesArray(state, array_values));
  for (std::size_t index = 0; index < round_tripped_values.size(); ++index) {
    assert(nearlyEqual(round_tripped_values[index], array_values[index]));
  }

  LegacyChunk synthetic_chunk;
  synthetic_chunk.programs.push_back(makeProgram("First Program", 0.10F));
  synthetic_chunk.programs.push_back(makeProgram("Second Program", 0.20F));

  const auto encoded = encodeLegacyChunk(synthetic_chunk);
  const auto decoded = decodeLegacyChunk(encoded);
  assert(decoded.ok());
  assert(decoded.chunk.has_value());

  const auto preset_bank = legacyPresetBankFromChunk(decoded.chunk.value());
  assert(!hasExpectedLegacyProgramCount(preset_bank));
  assert(preset_bank.presets.size() == 2U);
  assert(legacyPresetAt(preset_bank, 1) != nullptr);
  assert(legacyPresetAt(preset_bank, 1)->name == "Second Program");
  assert(legacyPresetIndexByName(preset_bank, "Second Program").has_value());
  assert(legacyPresetIndexByName(preset_bank, "Second Program").value() == 1U);

  auto explicit_values = decoded.chunk->programs[1].parameter_values;
  explicit_values[7] += 0.125F;

  const auto imported = importLegacyState(decoded.chunk.value(), 1, explicit_values);
  assert(imported.current_state_source ==
         LegacyCurrentStateSource::kExplicitParameters);
  assert(imported.selected_program_index.has_value());
  assert(imported.selected_program_index.value() == 1U);
  assert(imported.selected_program_name == "Second Program");
  assert(!imported.selected_program_matches_explicit_parameters);
  assert(imported.parameter_mismatches.size() == 1U);
  assert(imported.parameter_mismatches.front().id ==
         LegacyParameterId::kCompressAmount);
  assert(imported.parameter_mismatches.front().legacy_index == 7U);
  assert(nearlyEqual(imported.parameter_mismatches.front().selected_program_value,
                     decoded.chunk->programs[1].parameter_values[7]));
  assert(nearlyEqual(imported.parameter_mismatches.front().explicit_parameter_value,
                     explicit_values[7]));

  const auto imported_values = importedStateToArray(imported);
  assert(nearlyEqual(imported_values[7], explicit_values[7]));

  std::cout << "Legacy model smoke test passed\n";
  return 0;
}
