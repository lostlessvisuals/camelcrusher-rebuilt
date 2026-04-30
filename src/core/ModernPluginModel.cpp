#include "camelcrusher_recalled/ModernPluginModel.h"

#include <algorithm>
#include <array>

namespace camelcrusher_recalled {
namespace {

constexpr std::array<ModernPublicParameterDescriptor, kModernPublicParameterCount>
    kModernPublicParameters{{
        {0, LegacyParameterId::kDistOn, "dist_on", "Dist On",
         LegacyParameterValueKind::kSwitchLike},
        {1, LegacyParameterId::kDistMech, "dist_mech", "Dist Mech",
         LegacyParameterValueKind::kContinuousNormalized},
        {2, LegacyParameterId::kDistTube, "dist_tube", "Dist Tube",
         LegacyParameterValueKind::kContinuousNormalized},
        {3, LegacyParameterId::kMmFilterOn, "filter_on", "Filter On",
         LegacyParameterValueKind::kSwitchLike},
        {4, LegacyParameterId::kMmFilterCutoff, "filter_cutoff",
         "Filter Cutoff", LegacyParameterValueKind::kContinuousNormalized},
        {5, LegacyParameterId::kMmFilterRes, "filter_res", "Filter Res",
         LegacyParameterValueKind::kContinuousNormalized},
        {6, LegacyParameterId::kCompressOn, "compress_on", "Compress On",
         LegacyParameterValueKind::kSwitchLike},
        {7, LegacyParameterId::kCompressAmount, "compress_amount",
         "Compress Amount", LegacyParameterValueKind::kContinuousNormalized},
        {8, LegacyParameterId::kCompressMode, "compress_mode",
         "Compress Mode", LegacyParameterValueKind::kContinuousNormalized},
        {9, LegacyParameterId::kMasterOn, "master_on", "Master On",
         LegacyParameterValueKind::kSwitchLike},
        {10, LegacyParameterId::kMasterMix, "master_mix", "Master Mix",
         LegacyParameterValueKind::kContinuousNormalized},
        {11, LegacyParameterId::kMasterVolume, "master_volume",
         "Master Volume", LegacyParameterValueKind::kContinuousNormalized},
    }};

}  // namespace

std::span<const ModernPublicParameterDescriptor> modernPublicParameterDescriptors() {
  return kModernPublicParameters;
}

std::optional<std::size_t> modernPublicParameterIndex(
    const LegacyParameterId legacy_id) {
  for (const auto& descriptor : kModernPublicParameters) {
    if (descriptor.legacy_id == legacy_id) {
      return descriptor.public_index;
    }
  }

  return std::nullopt;
}

std::optional<LegacyParameterId> modernLegacyParameterIdForPublicIndex(
    const std::size_t public_index) {
  if (public_index >= kModernPublicParameters.size()) {
    return std::nullopt;
  }

  return kModernPublicParameters[public_index].legacy_id;
}

float clampModernNormalizedValue(const float value) {
  return std::clamp(value, 0.0F, 1.0F);
}

ModernPublicParameterArray modernPublicParameterArrayFromState(
    const LegacyState& state) {
  ModernPublicParameterArray public_values{};

  for (const auto& descriptor : kModernPublicParameters) {
    public_values[descriptor.public_index] =
        clampModernNormalizedValue(legacyStateValue(state, descriptor.legacy_id));
  }

  return public_values;
}

void applyModernPublicParameterArray(LegacyState& state,
                                     const std::span<const float> public_values) {
  const auto count = std::min(public_values.size(), kModernPublicParameters.size());
  for (std::size_t index = 0; index < count; ++index) {
    setLegacyStateValue(state, kModernPublicParameters[index].legacy_id,
                        clampModernNormalizedValue(public_values[index]));
  }
}

float modernPublicParameterValue(const LegacyState& state,
                                 const std::size_t public_index) {
  const auto legacy_id = modernLegacyParameterIdForPublicIndex(public_index);
  if (!legacy_id.has_value()) {
    return 0.0F;
  }

  return clampModernNormalizedValue(legacyStateValue(state, legacy_id.value()));
}

void setModernPublicParameterValue(LegacyState& state,
                                   const std::size_t public_index,
                                   const float normalized_value) {
  const auto legacy_id = modernLegacyParameterIdForPublicIndex(public_index);
  if (!legacy_id.has_value()) {
    return;
  }

  setLegacyStateValue(state, legacy_id.value(),
                      clampModernNormalizedValue(normalized_value));
}

ModernPluginState modernPluginStateFromLegacyImport(
    const LegacyImportedState& imported_state) {
  return ModernPluginState{
      .state = imported_state.current_state,
      .legacy_preset_bank = imported_state.preset_bank,
      .selected_legacy_program_index = imported_state.selected_program_index,
      .selected_legacy_program_name = imported_state.selected_program_name,
      .legacy_parameter_mismatches = imported_state.parameter_mismatches,
  };
}

}  // namespace camelcrusher_recalled
