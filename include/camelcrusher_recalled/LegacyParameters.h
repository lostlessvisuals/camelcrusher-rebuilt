#pragma once

#include "camelcrusher_recalled/LegacySchema.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace camelcrusher_recalled {

enum class LegacyParameterId : std::size_t {
  kDistOn = 0,
  kDistMech,
  kDistTube,
  kMmFilterOn,
  kMmFilterCutoff,
  kMmFilterRes,
  kCompressOn,
  kCompressAmount,
  kCompressMode,
  kMasterOn,
  kMasterMix,
  kMasterVolume,
  kUnused1,
  kUnused2,
  kUnused3,
  kUnused4,
  kUnused5,
};

enum class LegacyParameterUsage {
  kExposed,
  kReservedCompatibilitySlot,
};

enum class LegacyParameterValueKind {
  kContinuousNormalized,
  kSwitchLike,
};

struct LegacyParameterDescriptor {
  LegacyParameterId id;
  std::size_t legacy_index;
  std::string_view key;
  std::string_view display_name;
  LegacyParameterUsage usage;
  LegacyParameterValueKind value_kind;
};

std::span<const LegacyParameterDescriptor> legacyParameterDescriptors();
const LegacyParameterDescriptor& legacyParameterDescriptor(LegacyParameterId id);
std::optional<LegacyParameterId> legacyParameterIdFromLegacyIndex(
    std::size_t index);
std::optional<LegacyParameterId> legacyParameterIdFromName(std::string_view name);
bool isReservedCompatibilityParameter(LegacyParameterId id);
bool isSwitchLikeParameter(LegacyParameterId id);

}  // namespace camelcrusher_recalled
