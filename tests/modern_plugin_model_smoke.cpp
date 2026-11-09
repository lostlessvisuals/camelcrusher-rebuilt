#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyImport.h"
#include "camelcrusher_recalled/LegacyState.h"
#include "camelcrusher_recalled/ModernPluginModel.h"

#include "TestSupport.h"
#include <cmath>
#include <cstddef>
#include <iostream>

namespace {

using camelcrusher_recalled::decodeLegacyChunk;
using camelcrusher_recalled::encodeLegacyChunk;
using camelcrusher_recalled::importLegacyState;
using camelcrusher_recalled::modernLegacyParameterIdForPublicIndex;
using camelcrusher_recalled::modernPluginStateFromLegacyImport;
using camelcrusher_recalled::modernPublicParameterArrayFromState;
using camelcrusher_recalled::modernPublicParameterDescriptors;
using camelcrusher_recalled::modernPublicParameterIndex;
using camelcrusher_recalled::setModernPublicParameterValue;
using camelcrusher_recalled::ModernPluginState;
using camelcrusher_recalled::LegacyChunk;
using camelcrusher_recalled::LegacyParameterId;
using camelcrusher_recalled::LegacyProgram;
using camelcrusher_recalled::kModernPublicParameterCount;

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
  const auto descriptors = modernPublicParameterDescriptors();
  CC_CHECK(descriptors.size() == kModernPublicParameterCount);
  CC_CHECK(descriptors[0].legacy_id == LegacyParameterId::kDistOn);
  CC_CHECK(descriptors[11].legacy_id == LegacyParameterId::kMasterVolume);
  CC_CHECK(modernPublicParameterIndex(LegacyParameterId::kCompressAmount).has_value());
  CC_CHECK(modernPublicParameterIndex(LegacyParameterId::kCompressAmount).value() ==
         7U);
  CC_CHECK(!modernPublicParameterIndex(LegacyParameterId::kUnused1).has_value());
  CC_CHECK(modernLegacyParameterIdForPublicIndex(9).has_value());
  CC_CHECK(modernLegacyParameterIdForPublicIndex(9).value() ==
         LegacyParameterId::kMasterOn);

  LegacyChunk synthetic_chunk;
  synthetic_chunk.programs.push_back(makeProgram("First Program", 0.10F));
  synthetic_chunk.programs.push_back(makeProgram("Second Program", 0.20F));

  const auto encoded = encodeLegacyChunk(synthetic_chunk);
  const auto decoded = decodeLegacyChunk(encoded);
  CC_CHECK(decoded.ok());
  CC_CHECK(decoded.chunk.has_value());

  auto explicit_values = decoded.chunk->programs[1].parameter_values;
  explicit_values[10] = 1.25F;
  const auto imported = importLegacyState(decoded.chunk.value(), 1, explicit_values);
  ModernPluginState modern_state = modernPluginStateFromLegacyImport(imported);

  CC_CHECK(modern_state.selected_legacy_program_index.has_value());
  CC_CHECK(modern_state.selected_legacy_program_index.value() == 1U);
  CC_CHECK(modern_state.selected_legacy_program_name == "Second Program");
  CC_CHECK(modern_state.legacy_preset_bank.presets.size() == 2U);
  CC_CHECK(modern_state.legacy_parameter_mismatches.size() == 1U);
  CC_CHECK(modern_state.legacy_parameter_mismatches.front().id ==
         LegacyParameterId::kMasterMix);

  const auto public_values = modernPublicParameterArrayFromState(modern_state.state);
  CC_CHECK(public_values.size() == kModernPublicParameterCount);
  CC_CHECK(nearlyEqual(public_values[0], explicit_values[0]));
  CC_CHECK(nearlyEqual(public_values[7], explicit_values[7]));
  CC_CHECK(nearlyEqual(public_values[10], 1.0F));

  const auto reserved_before = modern_state.state.unused_1;
  setModernPublicParameterValue(modern_state.state, 0, -0.25F);
  setModernPublicParameterValue(modern_state.state, 10, 1.50F);
  CC_CHECK(nearlyEqual(modern_state.state.dist_on, 0.0F));
  CC_CHECK(nearlyEqual(modern_state.state.master_mix, 1.0F));
  CC_CHECK(nearlyEqual(modern_state.state.unused_1, reserved_before));

  std::cout << "Modern plugin model smoke test passed\n";
  return 0;
}
