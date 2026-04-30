#include "camelcrusher_recalled/ModernRuntimeBridge.h"

#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/ModernRuntime.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>
#include <string>

namespace camelcrusher_recalled {
namespace {

void clearTextError(CamelCrusherRuntimeBridgeTextError* error) {
  if (error == nullptr) {
    return;
  }

  error->offset = 0;
  error->message[0] = '\0';
}

void setTextError(CamelCrusherRuntimeBridgeTextError* error,
                  const std::size_t offset,
                  const std::string& message) {
  if (error == nullptr) {
    return;
  }

  error->offset = offset;
  const auto copy_size = std::min(
      message.size(),
      static_cast<std::size_t>(
          CAMELCRUSHER_RUNTIME_BRIDGE_MAX_ERROR_MESSAGE - 1));
  std::memcpy(error->message, message.data(), copy_size);
  error->message[copy_size] = '\0';
}

void writeBoundedString(const std::string& value,
                        char* destination,
                        const std::size_t capacity) {
  if (destination == nullptr || capacity == 0) {
    return;
  }

  const auto copy_size = std::min(value.size(), capacity - 1);
  std::memcpy(destination, value.data(), copy_size);
  destination[copy_size] = '\0';
}

void clearLegacyImportSummary(
    CamelCrusherRuntimeBridgeLegacyImportSummary* summary) {
  if (summary == nullptr) {
    return;
  }

  summary->selected_program_present = 0;
  summary->selected_program_index = 0;
  summary->selected_program_name[0] = '\0';
  summary->preset_count = 0;
  summary->mismatch_count = 0;
}

CamelCrusherRuntimeBridgeLegacyImportSummary makeLegacyImportSummary(
    const ModernPluginState& state) {
  CamelCrusherRuntimeBridgeLegacyImportSummary summary{};
  summary.selected_program_present =
      state.selected_legacy_program_index.has_value() ? 1 : 0;
  summary.selected_program_index =
      state.selected_legacy_program_index.value_or(0);
  writeBoundedString(state.selected_legacy_program_name,
                     summary.selected_program_name,
                     CAMELCRUSHER_RUNTIME_BRIDGE_MAX_PROGRAM_NAME);
  summary.preset_count = state.legacy_preset_bank.presets.size();
  summary.mismatch_count = state.legacy_parameter_mismatches.size();
  return summary;
}

}  // namespace
}  // namespace camelcrusher_recalled

using camelcrusher_recalled::LegacyParameterValueKind;
using camelcrusher_recalled::LegacyImportedState;
using camelcrusher_recalled::ModernAudioBlock;
using camelcrusher_recalled::ModernPluginRuntime;
using camelcrusher_recalled::ModernProcessSpec;
using camelcrusher_recalled::clearLegacyImportSummary;
using camelcrusher_recalled::clearTextError;
using camelcrusher_recalled::decodeLegacyChunk;
using camelcrusher_recalled::importLegacyState;
using camelcrusher_recalled::legacyParameterDescriptor;
using camelcrusher_recalled::legacyParameterIdFromLegacyIndex;
using camelcrusher_recalled::legacyStateValue;
using camelcrusher_recalled::makeLegacyImportSummary;
using camelcrusher_recalled::modernPublicParameterDescriptors;
using camelcrusher_recalled::setTextError;
using camelcrusher_recalled::writeBoundedString;

struct CamelCrusherRuntimeBridgeRuntime {
  ModernPluginRuntime runtime;
};

extern "C" {

size_t camelcrusher_runtime_bridge_public_parameter_count(void) {
  return modernPublicParameterDescriptors().size();
}

int camelcrusher_runtime_bridge_get_public_parameter_descriptor(
    const size_t public_index,
    CamelCrusherRuntimeBridgePublicParameterDescriptor* out_descriptor) {
  if (out_descriptor == nullptr) {
    return 0;
  }

  const auto descriptors = modernPublicParameterDescriptors();
  if (public_index >= descriptors.size()) {
    return 0;
  }

  const auto& descriptor = descriptors[public_index];
  const auto& legacy_descriptor = legacyParameterDescriptor(descriptor.legacy_id);

  out_descriptor->public_index = descriptor.public_index;
  out_descriptor->legacy_index = legacy_descriptor.legacy_index;
  out_descriptor->stable_id = descriptor.stable_id.data();
  out_descriptor->display_name = descriptor.display_name.data();
  out_descriptor->value_kind =
      descriptor.value_kind == LegacyParameterValueKind::kSwitchLike
          ? CAMELCRUSHER_RUNTIME_BRIDGE_VALUE_SWITCH_LIKE
          : CAMELCRUSHER_RUNTIME_BRIDGE_VALUE_CONTINUOUS_NORMALIZED;
  return 1;
}

CamelCrusherRuntimeBridgeRuntime* camelcrusher_runtime_bridge_runtime_create(
    void) {
  return new CamelCrusherRuntimeBridgeRuntime{};
}

void camelcrusher_runtime_bridge_runtime_destroy(
    CamelCrusherRuntimeBridgeRuntime* runtime) {
  delete runtime;
}

void camelcrusher_runtime_bridge_runtime_prepare(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    const CamelCrusherRuntimeBridgeProcessSpec spec) {
  if (runtime == nullptr) {
    return;
  }

  runtime->runtime.prepare(ModernProcessSpec{
      .sample_rate = spec.sample_rate,
      .max_block_size = spec.max_block_size,
      .channel_count = spec.channel_count,
  });
}

void camelcrusher_runtime_bridge_runtime_reset(
    CamelCrusherRuntimeBridgeRuntime* runtime) {
  if (runtime == nullptr) {
    return;
  }

  runtime->runtime.reset();
}

float camelcrusher_runtime_bridge_runtime_get_public_parameter(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    const size_t public_index) {
  if (runtime == nullptr) {
    return 0.0F;
  }

  return runtime->runtime.publicParameter(public_index);
}

void camelcrusher_runtime_bridge_runtime_set_public_parameter(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    const size_t public_index,
    const float normalized_value) {
  if (runtime == nullptr) {
    return;
  }

  runtime->runtime.setPublicParameter(public_index, normalized_value);
}

float camelcrusher_runtime_bridge_runtime_get_legacy_parameter(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    const size_t legacy_index) {
  if (runtime == nullptr) {
    return 0.0F;
  }

  const auto legacy_id = legacyParameterIdFromLegacyIndex(legacy_index);
  if (!legacy_id.has_value()) {
    return 0.0F;
  }

  return legacyStateValue(runtime->runtime.state().state, legacy_id.value());
}

size_t camelcrusher_runtime_bridge_runtime_preset_count(
    const CamelCrusherRuntimeBridgeRuntime* runtime) {
  if (runtime == nullptr) {
    return 0;
  }

  return runtime->runtime.state().legacy_preset_bank.presets.size();
}

size_t camelcrusher_runtime_bridge_runtime_mismatch_count(
    const CamelCrusherRuntimeBridgeRuntime* runtime) {
  if (runtime == nullptr) {
    return 0;
  }

  return runtime->runtime.state().legacy_parameter_mismatches.size();
}

int camelcrusher_runtime_bridge_runtime_get_selected_program_index(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    size_t* out_program_index) {
  if (runtime == nullptr || out_program_index == nullptr ||
      !runtime->runtime.state().selected_legacy_program_index.has_value()) {
    return 0;
  }

  *out_program_index =
      runtime->runtime.state().selected_legacy_program_index.value();
  return 1;
}

int camelcrusher_runtime_bridge_runtime_get_selected_program_name(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    char* out_name,
    const size_t out_name_capacity) {
  if (runtime == nullptr || out_name == nullptr || out_name_capacity == 0 ||
      !runtime->runtime.state().selected_legacy_program_index.has_value()) {
    if (out_name != nullptr && out_name_capacity > 0) {
      out_name[0] = '\0';
    }
    return 0;
  }

  writeBoundedString(runtime->runtime.state().selected_legacy_program_name,
                     out_name, out_name_capacity);
  return 1;
}

int camelcrusher_runtime_bridge_runtime_get_legacy_preset_name(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    const size_t program_index,
    char* out_name,
    const size_t out_name_capacity) {
  if (out_name != nullptr && out_name_capacity > 0) {
    out_name[0] = '\0';
  }

  if (runtime == nullptr || out_name == nullptr || out_name_capacity == 0) {
    return 0;
  }

  const auto& presets = runtime->runtime.state().legacy_preset_bank.presets;
  if (program_index >= presets.size()) {
    return 0;
  }

  writeBoundedString(presets[program_index].name, out_name, out_name_capacity);
  return 1;
}

int camelcrusher_runtime_bridge_runtime_select_legacy_preset(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    const size_t program_index,
    const int adopt_preset_state) {
  if (runtime == nullptr) {
    return 0;
  }

  return runtime->runtime.selectLegacyPreset(program_index,
                                             adopt_preset_state != 0)
             ? 1
             : 0;
}

int camelcrusher_runtime_bridge_runtime_import_legacy_chunk(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    const uint8_t* chunk_bytes,
    const size_t chunk_size,
    const int32_t program_number,
    const float* explicit_parameter_values,
    const size_t explicit_parameter_count,
    CamelCrusherRuntimeBridgeLegacyImportSummary* out_summary,
    CamelCrusherRuntimeBridgeTextError* out_error) {
  clearTextError(out_error);
  clearLegacyImportSummary(out_summary);

  if (runtime == nullptr || chunk_bytes == nullptr) {
    setTextError(out_error, 0, "Runtime and chunk bytes are required");
    return 0;
  }

  const auto decoded = decodeLegacyChunk(std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(chunk_bytes), chunk_size));
  if (!decoded.ok() || !decoded.chunk.has_value()) {
    const auto offset =
        decoded.error.has_value() ? decoded.error->offset : static_cast<size_t>(0);
    const auto message = decoded.error.has_value()
                             ? decoded.error->message
                             : std::string("Chunk decode failed");
    setTextError(out_error, offset, message);
    return 0;
  }

  std::optional<int> selected_program_number;
  if (program_number >= 0) {
    selected_program_number = static_cast<int>(program_number);
  }

  std::span<const float> explicit_values;
  if (explicit_parameter_values != nullptr && explicit_parameter_count > 0) {
    explicit_values =
        std::span<const float>(explicit_parameter_values, explicit_parameter_count);
  }

  const LegacyImportedState imported_state = importLegacyState(
      decoded.chunk.value(), selected_program_number, explicit_values);
  runtime->runtime.importLegacyState(imported_state);

  if (out_summary != nullptr) {
    *out_summary = makeLegacyImportSummary(runtime->runtime.state());
  }

  return 1;
}

size_t camelcrusher_runtime_bridge_runtime_state_size(
    const CamelCrusherRuntimeBridgeRuntime* runtime) {
  if (runtime == nullptr) {
    return 0;
  }

  return runtime->runtime.saveState().size();
}

int camelcrusher_runtime_bridge_runtime_save_state(
    const CamelCrusherRuntimeBridgeRuntime* runtime,
    uint8_t* out_bytes,
    const size_t out_bytes_capacity,
    size_t* out_bytes_written) {
  if (runtime == nullptr || out_bytes_written == nullptr) {
    return 0;
  }

  const auto bytes = runtime->runtime.saveState();
  *out_bytes_written = bytes.size();
  if (out_bytes == nullptr || out_bytes_capacity < bytes.size()) {
    return 0;
  }

  std::memcpy(out_bytes, bytes.data(), bytes.size());
  return 1;
}

int camelcrusher_runtime_bridge_runtime_load_state(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    const uint8_t* bytes,
    const size_t byte_count,
    CamelCrusherRuntimeBridgeTextError* out_error) {
  clearTextError(out_error);

  if (runtime == nullptr || bytes == nullptr) {
    setTextError(out_error, 0, "Runtime and state bytes are required");
    return 0;
  }

  const auto result = runtime->runtime.loadState(std::span<const std::byte>(
      reinterpret_cast<const std::byte*>(bytes), byte_count));
  if (!result.ok() || result.error.has_value()) {
    const auto offset =
        result.error.has_value() ? result.error->offset : static_cast<size_t>(0);
    const auto message = result.error.has_value()
                             ? result.error->message
                             : std::string("State decode failed");
    setTextError(out_error, offset, message);
    return 0;
  }

  return 1;
}

void camelcrusher_runtime_bridge_runtime_process_stereo(
    CamelCrusherRuntimeBridgeRuntime* runtime,
    float* left,
    float* right,
    const size_t frame_count) {
  if (runtime == nullptr || left == nullptr || right == nullptr ||
      frame_count == 0) {
    return;
  }

  runtime->runtime.processBlock(ModernAudioBlock{
      .left = std::span<float>(left, frame_count),
      .right = std::span<float>(right, frame_count),
  });
}

}  // extern "C"
