#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/ModernRuntimeBridge.h"

#include <algorithm>
#include "TestSupport.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

using camelcrusher_recalled::encodeLegacyChunk;
using camelcrusher_recalled::LegacyChunk;
using camelcrusher_recalled::LegacyProgram;

LegacyProgram makeProgram(const char* name, const float seed) {
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
  CC_CHECK(camelcrusher_runtime_bridge_public_parameter_count() == 12U);

  CamelCrusherRuntimeBridgePublicParameterDescriptor descriptor{};
  CC_CHECK(camelcrusher_runtime_bridge_get_public_parameter_descriptor(0,
                                                                     &descriptor));
  CC_CHECK(descriptor.public_index == 0U);
  CC_CHECK(descriptor.legacy_index == 0U);
  CC_CHECK(std::strcmp(descriptor.stable_id, "dist_on") == 0);
  CC_CHECK(descriptor.value_kind ==
         CAMELCRUSHER_RUNTIME_BRIDGE_VALUE_SWITCH_LIKE);

  LegacyChunk synthetic_chunk;
  synthetic_chunk.programs.push_back(makeProgram("Neutral", 0.10F));
  synthetic_chunk.programs.push_back(makeProgram("Driven", 0.20F));
  const auto encoded_chunk = encodeLegacyChunk(synthetic_chunk);

  std::vector<uint8_t> chunk_bytes(encoded_chunk.size());
  std::memcpy(chunk_bytes.data(), encoded_chunk.data(), encoded_chunk.size());

  std::vector<float> explicit_values(
      synthetic_chunk.programs[1].parameter_values.begin(),
      synthetic_chunk.programs[1].parameter_values.end());

  auto* runtime = camelcrusher_runtime_bridge_runtime_create();
  CC_CHECK(runtime != nullptr);

  camelcrusher_runtime_bridge_runtime_prepare(
      runtime,
      CamelCrusherRuntimeBridgeProcessSpec{
          .sample_rate = 48000.0,
          .max_block_size = 32,
          .channel_count = 2,
      });

  CamelCrusherRuntimeBridgeLegacyImportSummary import_summary{};
  CamelCrusherRuntimeBridgeTextError error{};
  CC_CHECK(camelcrusher_runtime_bridge_runtime_import_legacy_chunk(
      runtime, chunk_bytes.data(), chunk_bytes.size(), 1, explicit_values.data(),
      explicit_values.size(), &import_summary, &error));
  CC_CHECK(import_summary.selected_program_present == 1);
  CC_CHECK(import_summary.selected_program_index == 1U);
  CC_CHECK(std::strcmp(import_summary.selected_program_name, "Driven") == 0);
  CC_CHECK(import_summary.preset_count == 2U);
  CC_CHECK(import_summary.mismatch_count == 0U);

  size_t selected_index = 0;
  char selected_name[CAMELCRUSHER_RUNTIME_BRIDGE_MAX_PROGRAM_NAME]{};
  CC_CHECK(camelcrusher_runtime_bridge_runtime_get_selected_program_index(
      runtime, &selected_index));
  CC_CHECK(selected_index == 1U);
  CC_CHECK(camelcrusher_runtime_bridge_runtime_get_selected_program_name(
      runtime, selected_name, sizeof(selected_name)));
  CC_CHECK(std::strcmp(selected_name, "Driven") == 0);

  CC_CHECK(nearlyEqual(
      camelcrusher_runtime_bridge_runtime_get_public_parameter(runtime, 10),
      explicit_values[10]));
  CC_CHECK(nearlyEqual(
      camelcrusher_runtime_bridge_runtime_get_legacy_parameter(runtime, 12),
      explicit_values[12]));

  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 0, 1.0F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 1, 0.85F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 2, 0.70F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 3, 1.0F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 4, 0.20F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 5, 0.55F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 6, 1.0F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 7, 0.75F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 8, 0.35F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 9, 1.0F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 10, 1.25F);
  camelcrusher_runtime_bridge_runtime_set_public_parameter(runtime, 11, 0.82F);
  CC_CHECK(nearlyEqual(
      camelcrusher_runtime_bridge_runtime_get_public_parameter(runtime, 10), 1.0F));
  CC_CHECK(camelcrusher_runtime_bridge_runtime_mismatch_count(runtime) > 0U);

  const auto state_size = camelcrusher_runtime_bridge_runtime_state_size(runtime);
  CC_CHECK(state_size > 0U);
  std::vector<uint8_t> state_bytes(state_size);
  size_t bytes_written = 0;
  CC_CHECK(camelcrusher_runtime_bridge_runtime_save_state(
      runtime, state_bytes.data(), state_bytes.size(), &bytes_written));
  CC_CHECK(bytes_written == state_size);

  auto* restored_runtime = camelcrusher_runtime_bridge_runtime_create();
  CC_CHECK(restored_runtime != nullptr);
  camelcrusher_runtime_bridge_runtime_prepare(
      restored_runtime,
      CamelCrusherRuntimeBridgeProcessSpec{
          .sample_rate = 48000.0,
          .max_block_size = 32,
          .channel_count = 2,
      });
  CC_CHECK(camelcrusher_runtime_bridge_runtime_load_state(
      restored_runtime, state_bytes.data(), state_bytes.size(), &error));
  CC_CHECK(camelcrusher_runtime_bridge_runtime_mismatch_count(restored_runtime) > 0U);
  CC_CHECK(nearlyEqual(
      camelcrusher_runtime_bridge_runtime_get_public_parameter(restored_runtime, 0),
      1.0F));
  CC_CHECK(nearlyEqual(
      camelcrusher_runtime_bridge_runtime_get_public_parameter(restored_runtime, 10),
      1.0F));
  CC_CHECK(nearlyEqual(
      camelcrusher_runtime_bridge_runtime_get_public_parameter(restored_runtime, 11),
      0.82F));

  std::vector<float> left(16, 0.0F);
  std::vector<float> right(16, 0.0F);
  left[0] = 0.7F;
  right[0] = 0.7F;
  camelcrusher_runtime_bridge_runtime_process_stereo(
      restored_runtime, left.data(), right.data(), left.size());
  CC_CHECK(!nearlyEqual(left[0], 0.7F));
  CC_CHECK(!nearlyEqual(right[0], 0.7F));
  CC_CHECK(peakMagnitude(left) <= 1.0F);
  CC_CHECK(peakMagnitude(right) <= 1.0F);

  char preset_name[CAMELCRUSHER_RUNTIME_BRIDGE_MAX_PROGRAM_NAME]{};
  CC_CHECK(camelcrusher_runtime_bridge_runtime_get_legacy_preset_name(
      restored_runtime, 0, preset_name, sizeof(preset_name)));
  CC_CHECK(std::strcmp(preset_name, "Neutral") == 0);

  CC_CHECK(camelcrusher_runtime_bridge_runtime_select_legacy_preset(
      restored_runtime, 0, 1));
  CC_CHECK(camelcrusher_runtime_bridge_runtime_mismatch_count(restored_runtime) == 0U);
  CC_CHECK(camelcrusher_runtime_bridge_runtime_select_legacy_preset(
      restored_runtime, 1, 0));
  CC_CHECK(camelcrusher_runtime_bridge_runtime_mismatch_count(restored_runtime) > 0U);

  camelcrusher_runtime_bridge_runtime_destroy(runtime);
  camelcrusher_runtime_bridge_runtime_destroy(restored_runtime);

  return 0;
}
