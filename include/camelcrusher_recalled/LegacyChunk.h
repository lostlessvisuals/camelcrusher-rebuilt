#pragma once

#include "camelcrusher_recalled/LegacySchema.h"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace camelcrusher_recalled {

struct LegacyProgram {
  float record_marker = 1.0F;
  std::string name;
  std::array<float, kLegacyParameterCount> parameter_values{};
};

struct LegacyChunk {
  std::vector<LegacyProgram> programs;
};

struct LegacyChunkDecodeError {
  std::size_t offset = 0;
  std::string message;
};

struct LegacyChunkDecodeResult {
  std::optional<LegacyChunk> chunk;
  std::optional<LegacyChunkDecodeError> error;

  [[nodiscard]] bool ok() const { return chunk.has_value(); }
};

LegacyChunkDecodeResult decodeLegacyChunk(std::span<const std::byte> bytes);
LegacyChunkDecodeResult decodeLegacyChunkHex(std::string_view hex);
std::vector<std::byte> encodeLegacyChunk(const LegacyChunk& chunk);
bool isLikelyLegacyChunk(const LegacyChunk& chunk);

}  // namespace camelcrusher_recalled
