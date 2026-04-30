#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace camelcrusher_recalled {

struct LegacyParameterSpec {
  int id;
  std::string_view name;
};

inline constexpr int kLegacyUniqueId = 1130447730;
inline constexpr std::string_view kLegacyUniqueIdFourCC = "CaCr";
inline constexpr int kLegacyParameterCount = 17;
inline constexpr int kLegacyProgramCount = 20;

inline constexpr std::array<LegacyParameterSpec, kLegacyParameterCount>
    kLegacyParameterSpecs{{
        {0, "DistOn"},
        {1, "DistMech"},
        {2, "DistTube"},
        {3, "MmFilterOn"},
        {4, "MmFilterCutoff"},
        {5, "MmFilterRes"},
        {6, "CompressOn"},
        {7, "CompressAmount"},
        {8, "CompressMode"},
        {9, "MasterOn"},
        {10, "MasterMix"},
        {11, "MasterVolume"},
        {12, "Unused1"},
        {13, "Unused2"},
        {14, "Unused3"},
        {15, "Unused4"},
        {16, "Unused5"},
    }};

std::optional<std::size_t> legacyParameterIndexByName(std::string_view name);
bool validateLegacyParameterOrdering(std::span<const std::string_view> names);

}  // namespace camelcrusher_recalled
