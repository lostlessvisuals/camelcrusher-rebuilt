#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyImport.h"
#include "camelcrusher_recalled/LegacyParameters.h"
#include "camelcrusher_recalled/LegacyPresetBank.h"
#include "camelcrusher_recalled/LegacySchema.h"
#include "camelcrusher_recalled/LegacyState.h"

#include "TestSupport.h"
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
  CC_CHECK(descriptors.size() == kLegacyParameterCount);
  CC_CHECK(descriptors[0].id == LegacyParameterId::kDistOn);
  CC_CHECK(descriptors[0].legacy_index == 0U);
  CC_CHECK(legacyParameterDescriptor(LegacyParameterId::kMasterVolume).legacy_index ==
         11U);
  for (std::size_t index = 0; index < descriptors.size(); ++index) {
    CC_CHECK(descriptors[index].legacy_index == index);
    CC_CHECK(descriptors[index].key == kLegacyParameterSpecs[index].name);
  }
  CC_CHECK(legacyParameterIdFromName("CompressAmount").has_value());
  CC_CHECK(legacyParameterIdFromName("CompressAmount").value() ==
         LegacyParameterId::kCompressAmount);
  CC_CHECK(isSwitchLikeParameter(LegacyParameterId::kDistOn));
  CC_CHECK(!isSwitchLikeParameter(LegacyParameterId::kMasterMix));
  CC_CHECK(isReservedCompatibilityParameter(LegacyParameterId::kUnused5));

  LegacyParameterArray array_values{};
  for (std::size_t index = 0; index < array_values.size(); ++index) {
    array_values[index] = static_cast<float>(index) / 16.0F;
  }

  const auto state = legacyStateFromArray(array_values);
  const auto round_tripped_values = legacyStateToArray(state);
  CC_CHECK(legacyStateMatchesArray(state, array_values));
  for (std::size_t index = 0; index < round_tripped_values.size(); ++index) {
    CC_CHECK(nearlyEqual(round_tripped_values[index], array_values[index]));
  }

  LegacyChunk synthetic_chunk;
  synthetic_chunk.programs.push_back(makeProgram("First Program", 0.10F));
  synthetic_chunk.programs.push_back(makeProgram("Second Program", 0.20F));

  const auto encoded = encodeLegacyChunk(synthetic_chunk);
  const auto decoded = decodeLegacyChunk(encoded);
  CC_CHECK(decoded.ok());
  CC_CHECK(decoded.chunk.has_value());

  const auto preset_bank = legacyPresetBankFromChunk(decoded.chunk.value());
  CC_CHECK(!hasExpectedLegacyProgramCount(preset_bank));
  CC_CHECK(preset_bank.presets.size() == 2U);
  CC_CHECK(legacyPresetAt(preset_bank, 1) != nullptr);
  CC_CHECK(legacyPresetAt(preset_bank, 1)->name == "Second Program");
  CC_CHECK(legacyPresetIndexByName(preset_bank, "Second Program").has_value());
  CC_CHECK(legacyPresetIndexByName(preset_bank, "Second Program").value() == 1U);

  auto explicit_values = decoded.chunk->programs[1].parameter_values;
  explicit_values[7] += 0.125F;

  const auto imported = importLegacyState(decoded.chunk.value(), 1, explicit_values);
  CC_CHECK(imported.current_state_source ==
         LegacyCurrentStateSource::kExplicitParameters);
  CC_CHECK(imported.selected_program_index.has_value());
  CC_CHECK(imported.selected_program_index.value() == 1U);
  CC_CHECK(imported.selected_program_name == "Second Program");
  CC_CHECK(!imported.selected_program_matches_explicit_parameters);
  CC_CHECK(imported.parameter_mismatches.size() == 1U);
  CC_CHECK(imported.parameter_mismatches.front().id ==
         LegacyParameterId::kCompressAmount);
  CC_CHECK(imported.parameter_mismatches.front().legacy_index == 7U);
  CC_CHECK(nearlyEqual(imported.parameter_mismatches.front().selected_program_value,
                     decoded.chunk->programs[1].parameter_values[7]));
  CC_CHECK(nearlyEqual(imported.parameter_mismatches.front().explicit_parameter_value,
                     explicit_values[7]));

  const auto imported_values = importedStateToArray(imported);
  CC_CHECK(nearlyEqual(imported_values[7], explicit_values[7]));

  std::cout << "Legacy model smoke test passed\n";
  return 0;
}
