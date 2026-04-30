#pragma once

#include "camelcrusher_recalled/ModernProcessor.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace camelcrusher_recalled {

struct ModernStateDecodeError {
  std::size_t offset = 0;
  std::string message;
};

struct ModernStateDecodeResult {
  std::optional<ModernPluginState> state;
  std::optional<ModernStateDecodeError> error;

  [[nodiscard]] bool ok() const { return state.has_value(); }
};

ModernPluginState makeDefaultModernPluginState();
std::vector<std::byte> serializeModernPluginState(const ModernPluginState& state);
ModernStateDecodeResult deserializeModernPluginState(
    std::span<const std::byte> bytes);

class ModernPluginRuntime {
 public:
  void prepare(const ModernProcessSpec& spec);
  void reset();

  const ModernPluginState& state() const;
  ModernPublicParameterArray publicParameters() const;
  float publicParameter(std::size_t public_index) const;
  void setPublicParameter(std::size_t public_index, float normalized_value);

  void replaceState(const ModernPluginState& state);
  void importLegacyState(const LegacyImportedState& imported_state);
  void clearSelectedLegacyPreset();
  bool selectLegacyPreset(std::size_t program_index, bool adopt_preset_state);

  std::vector<std::byte> saveState() const;
  ModernStateDecodeResult loadState(std::span<const std::byte> bytes);

  void processBlock(ModernAudioBlock block);

 private:
  void refreshSelectedProgramDiagnostics();

  ModernPluginState state_ = makeDefaultModernPluginState();
  ModernProcessor processor_;
};

}  // namespace camelcrusher_recalled
