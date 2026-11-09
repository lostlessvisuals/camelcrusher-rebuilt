#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyImport.h"
#include "camelcrusher_recalled/ModernRuntime.h"

#include <algorithm>
#include "TestSupport.h"
#include <cmath>
#include <iostream>
#include <vector>

namespace {

using camelcrusher_recalled::decodeLegacyChunk;
using camelcrusher_recalled::encodeLegacyChunk;
using camelcrusher_recalled::importLegacyState;
using camelcrusher_recalled::LegacyChunk;
using camelcrusher_recalled::LegacyParameterId;
using camelcrusher_recalled::LegacyProgram;
using camelcrusher_recalled::ModernAudioBlock;
using camelcrusher_recalled::ModernPluginRuntime;
using camelcrusher_recalled::ModernProcessSpec;
using camelcrusher_recalled::deserializeModernPluginState;

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

bool nearlyEqual(const float left, const float right,
                 const float epsilon = 0.000001F) {
  return std::fabs(left - right) < epsilon;
}

float peakMagnitude(const std::vector<float>& samples) {
  float peak = 0.0F;
  for (const auto sample : samples) {
    peak = std::max(peak, std::fabs(sample));
  }
  return peak;
}

}  // namespace

int main() {
  ModernPluginRuntime default_runtime;
  CC_CHECK(!default_runtime.state().selected_legacy_program_index.has_value());
  CC_CHECK(default_runtime.state().legacy_preset_bank.presets.size() == 20U);
  CC_CHECK(default_runtime.state().legacy_preset_bank.presets.front().name ==
         "Annihilate");
  CC_CHECK(default_runtime.state().legacy_preset_bank.presets.back().name ==
         "Turn it to 11");

  std::cerr << "checkpoint: build synthetic chunk\n";
  LegacyChunk synthetic_chunk;
  synthetic_chunk.programs.push_back(makeProgram("Neutral", 0.10F));
  synthetic_chunk.programs.push_back(makeProgram("Driven", 0.20F));

  const auto encoded = encodeLegacyChunk(synthetic_chunk);
  const auto decoded = decodeLegacyChunk(encoded);
  CC_CHECK(decoded.ok());
  CC_CHECK(decoded.chunk.has_value());

  auto explicit_values = decoded.chunk->programs[1].parameter_values;

  const auto imported = importLegacyState(decoded.chunk.value(), 1, explicit_values);

  std::cerr << "checkpoint: import runtime state\n";
  ModernPluginRuntime runtime;
  runtime.prepare(ModernProcessSpec{
      .sample_rate = 48000.0,
      .max_block_size = 16,
      .channel_count = 2,
  });
  runtime.importLegacyState(imported);

  CC_CHECK(runtime.state().selected_legacy_program_index.has_value());
  CC_CHECK(runtime.state().selected_legacy_program_index.value() == 1U);
  CC_CHECK(runtime.state().legacy_preset_bank.presets.size() == 2U);
  CC_CHECK(runtime.state().legacy_parameter_mismatches.empty());
  CC_CHECK(nearlyEqual(runtime.publicParameter(10), explicit_values[10]));
  CC_CHECK(nearlyEqual(runtime.state().state.unused_1, explicit_values[12]));

  std::cerr << "checkpoint: drive public parameters\n";
  runtime.setPublicParameter(0, 1.0F);
  runtime.setPublicParameter(1, 0.85F);
  runtime.setPublicParameter(2, 0.70F);
  runtime.setPublicParameter(3, 1.0F);
  runtime.setPublicParameter(4, 0.20F);
  runtime.setPublicParameter(5, 0.55F);
  runtime.setPublicParameter(6, 1.0F);
  runtime.setPublicParameter(7, 0.75F);
  runtime.setPublicParameter(8, 0.35F);
  runtime.setPublicParameter(9, 1.0F);
  runtime.setPublicParameter(10, 1.25F);
  runtime.setPublicParameter(11, 0.82F);

  CC_CHECK(nearlyEqual(runtime.publicParameter(10), 1.0F));
  CC_CHECK(nearlyEqual(runtime.state().state.master_mix, 1.0F));
  CC_CHECK(nearlyEqual(runtime.state().state.unused_1, explicit_values[12]));
  CC_CHECK(!runtime.state().legacy_parameter_mismatches.empty());

  std::cerr << "checkpoint: save and decode state\n";
  const auto saved_state = runtime.saveState();
  auto decoded_state = deserializeModernPluginState(saved_state);
  CC_CHECK(decoded_state.ok());
  CC_CHECK(decoded_state.state.has_value());
  CC_CHECK(decoded_state.state->selected_legacy_program_index.has_value());
  CC_CHECK(decoded_state.state->selected_legacy_program_index.value() == 1U);
  CC_CHECK(decoded_state.state->legacy_preset_bank.presets.size() == 2U);
  CC_CHECK(nearlyEqual(decoded_state.state->state.master_mix, 1.0F));

  std::cerr << "checkpoint: load runtime state\n";
  ModernPluginRuntime restored_runtime;
  restored_runtime.prepare(ModernProcessSpec{
      .sample_rate = 48000.0,
      .max_block_size = 16,
      .channel_count = 2,
  });
  const auto load_result = restored_runtime.loadState(saved_state);
  CC_CHECK(load_result.ok());
  CC_CHECK(!restored_runtime.state().legacy_parameter_mismatches.empty());

  std::cerr << "checkpoint: process driven audio\n";
  std::vector<float> left(16, 0.0F);
  std::vector<float> right(16, 0.0F);
  left[0] = 0.7F;
  right[0] = 0.7F;

  restored_runtime.processBlock(ModernAudioBlock{
      .left = left,
      .right = right,
  });

  CC_CHECK(!nearlyEqual(left[0], 0.7F));
  CC_CHECK(!nearlyEqual(right[0], 0.7F));
  CC_CHECK(peakMagnitude(left) <= 1.0F);
  CC_CHECK(peakMagnitude(right) <= 1.0F);

  std::cerr << "checkpoint: preset switching\n";
  CC_CHECK(restored_runtime.selectLegacyPreset(0, true));
  CC_CHECK(restored_runtime.state().selected_legacy_program_name == "Neutral");
  CC_CHECK(restored_runtime.state().legacy_parameter_mismatches.empty());

  CC_CHECK(restored_runtime.selectLegacyPreset(1, false));
  CC_CHECK(restored_runtime.state().selected_legacy_program_name == "Driven");
  CC_CHECK(!restored_runtime.state().legacy_parameter_mismatches.empty());

  CC_CHECK(restored_runtime.selectLegacyPreset(1, true));
  CC_CHECK(restored_runtime.state().legacy_parameter_mismatches.empty());

  std::cerr << "checkpoint: done\n";
  std::cout << "Modern runtime smoke test passed\n";
  return 0;
}
