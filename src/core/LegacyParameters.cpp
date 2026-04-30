#include "camelcrusher_recalled/LegacyParameters.h"

#include <array>

namespace camelcrusher_recalled {
namespace {

constexpr std::array<LegacyParameterDescriptor, kLegacyParameterCount>
    kLegacyDescriptors{{
        {LegacyParameterId::kDistOn, 0, "DistOn", "DistOn",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kSwitchLike},
        {LegacyParameterId::kDistMech, 1, "DistMech", "DistMech",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kDistTube, 2, "DistTube", "DistTube",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kMmFilterOn, 3, "MmFilterOn", "MmFilterOn",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kSwitchLike},
        {LegacyParameterId::kMmFilterCutoff, 4, "MmFilterCutoff",
         "MmFilterCutoff", LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kMmFilterRes, 5, "MmFilterRes", "MmFilterRes",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kCompressOn, 6, "CompressOn", "CompressOn",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kSwitchLike},
        {LegacyParameterId::kCompressAmount, 7, "CompressAmount",
         "CompressAmount", LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kCompressMode, 8, "CompressMode", "CompressMode",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kMasterOn, 9, "MasterOn", "MasterOn",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kSwitchLike},
        {LegacyParameterId::kMasterMix, 10, "MasterMix", "MasterMix",
         LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kMasterVolume, 11, "MasterVolume",
         "MasterVolume", LegacyParameterUsage::kExposed,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kUnused1, 12, "Unused1", "Unused1",
         LegacyParameterUsage::kReservedCompatibilitySlot,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kUnused2, 13, "Unused2", "Unused2",
         LegacyParameterUsage::kReservedCompatibilitySlot,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kUnused3, 14, "Unused3", "Unused3",
         LegacyParameterUsage::kReservedCompatibilitySlot,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kUnused4, 15, "Unused4", "Unused4",
         LegacyParameterUsage::kReservedCompatibilitySlot,
         LegacyParameterValueKind::kContinuousNormalized},
        {LegacyParameterId::kUnused5, 16, "Unused5", "Unused5",
         LegacyParameterUsage::kReservedCompatibilitySlot,
         LegacyParameterValueKind::kContinuousNormalized},
    }};

}  // namespace

std::span<const LegacyParameterDescriptor> legacyParameterDescriptors() {
  return kLegacyDescriptors;
}

const LegacyParameterDescriptor& legacyParameterDescriptor(
    const LegacyParameterId id) {
  return kLegacyDescriptors[static_cast<std::size_t>(id)];
}

std::optional<LegacyParameterId> legacyParameterIdFromLegacyIndex(
    const std::size_t index) {
  if (index >= kLegacyDescriptors.size()) {
    return std::nullopt;
  }

  return kLegacyDescriptors[index].id;
}

std::optional<LegacyParameterId> legacyParameterIdFromName(
    const std::string_view name) {
  const auto index = legacyParameterIndexByName(name);
  if (!index.has_value()) {
    return std::nullopt;
  }

  return legacyParameterIdFromLegacyIndex(index.value());
}

bool isReservedCompatibilityParameter(const LegacyParameterId id) {
  return legacyParameterDescriptor(id).usage ==
         LegacyParameterUsage::kReservedCompatibilitySlot;
}

bool isSwitchLikeParameter(const LegacyParameterId id) {
  return legacyParameterDescriptor(id).value_kind ==
         LegacyParameterValueKind::kSwitchLike;
}

}  // namespace camelcrusher_recalled
