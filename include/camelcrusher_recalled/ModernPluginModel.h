#pragma once

#include "camelcrusher_recalled/LegacyImport.h"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace camelcrusher_recalled {

inline constexpr std::size_t kModernPublicParameterCount = 12;

struct ModernPublicParameterDescriptor {
  std::size_t public_index = 0;
  LegacyParameterId legacy_id = LegacyParameterId::kDistOn;
  std::string_view stable_id;
  std::string_view display_name;
  LegacyParameterValueKind value_kind = LegacyParameterValueKind::kContinuousNormalized;
};

using ModernPublicParameterArray =
    std::array<float, kModernPublicParameterCount>;

struct ModernPluginState {
  LegacyState state;
  LegacyPresetBank legacy_preset_bank;
  std::optional<std::size_t> selected_legacy_program_index;
  std::string selected_legacy_program_name;
  std::vector<LegacyParameterMismatch> legacy_parameter_mismatches;
};

std::span<const ModernPublicParameterDescriptor> modernPublicParameterDescriptors();
std::optional<std::size_t> modernPublicParameterIndex(LegacyParameterId legacy_id);
std::optional<LegacyParameterId> modernLegacyParameterIdForPublicIndex(
    std::size_t public_index);

float clampModernNormalizedValue(float value);
ModernPublicParameterArray modernPublicParameterArrayFromState(
    const LegacyState& state);
void applyModernPublicParameterArray(LegacyState& state,
                                     std::span<const float> public_values);
float modernPublicParameterValue(const LegacyState& state,
                                 std::size_t public_index);
void setModernPublicParameterValue(LegacyState& state,
                                   std::size_t public_index,
                                   float normalized_value);

ModernPluginState modernPluginStateFromLegacyImport(
    const LegacyImportedState& imported_state);

}  // namespace camelcrusher_recalled
