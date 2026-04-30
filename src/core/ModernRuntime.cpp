#include "camelcrusher_recalled/ModernRuntime.h"

#include "DefaultLegacyFactoryBank.h"

#include <array>
#include <cstring>
#include <limits>

namespace camelcrusher_recalled {
namespace {

constexpr std::array<std::byte, 8> kModernStateMagic{
    std::byte{'C'}, std::byte{'C'}, std::byte{'R'}, std::byte{'M'},
    std::byte{'O'}, std::byte{'D'}, std::byte{'1'}, std::byte{0},
};
constexpr std::uint32_t kModernStateVersion = 1;
constexpr std::uint32_t kNoSelectedProgram = std::numeric_limits<std::uint32_t>::max();

template <typename T>
void writePod(std::vector<std::byte>& bytes, const T& value) {
  const auto* raw = reinterpret_cast<const std::byte*>(&value);
  bytes.insert(bytes.end(), raw, raw + sizeof(T));
}

template <typename T>
bool readPod(std::span<const std::byte> bytes, std::size_t offset, T& value) {
  if (offset + sizeof(T) > bytes.size()) {
    return false;
  }

  std::memcpy(&value, bytes.data() + offset, sizeof(T));
  return true;
}

void writeString(std::vector<std::byte>& bytes, const std::string& value) {
  const auto length = static_cast<std::uint32_t>(value.size());
  writePod(bytes, length);
  bytes.insert(bytes.end(),
               reinterpret_cast<const std::byte*>(value.data()),
               reinterpret_cast<const std::byte*>(value.data()) + value.size());
}

bool readString(std::span<const std::byte> bytes,
                std::size_t& offset,
                std::string& value) {
  std::uint32_t length = 0;
  if (!readPod(bytes, offset, length)) {
    return false;
  }
  offset += sizeof(std::uint32_t);

  if (offset + length > bytes.size()) {
    return false;
  }

  value.assign(reinterpret_cast<const char*>(bytes.data() + offset), length);
  offset += length;
  return true;
}

void writeLegacyState(std::vector<std::byte>& bytes, const LegacyState& state) {
  const auto values = legacyStateToArray(state);
  for (const auto value : values) {
    writePod(bytes, value);
  }
}

bool readLegacyState(std::span<const std::byte> bytes,
                     std::size_t& offset,
                     LegacyState& state) {
  LegacyParameterArray values{};
  for (auto& value : values) {
    if (!readPod(bytes, offset, value)) {
      return false;
    }
    offset += sizeof(float);
  }

  state = legacyStateFromArray(values);
  return true;
}

std::vector<LegacyParameterMismatch> buildParameterMismatches(
    const LegacyState& current_state,
    const LegacyPreset* selected_preset) {
  std::vector<LegacyParameterMismatch> mismatches;
  if (selected_preset == nullptr) {
    return mismatches;
  }

  const auto current_values = legacyStateToArray(current_state);
  const auto preset_values = legacyStateToArray(selected_preset->state);

  for (const auto& descriptor : legacyParameterDescriptors()) {
    if (current_values[descriptor.legacy_index] !=
        preset_values[descriptor.legacy_index]) {
      mismatches.push_back(LegacyParameterMismatch{
          .id = descriptor.id,
          .legacy_index = descriptor.legacy_index,
          .selected_program_value = preset_values[descriptor.legacy_index],
          .explicit_parameter_value = current_values[descriptor.legacy_index],
      });
    }
  }

  return mismatches;
}

}  // namespace

ModernPluginState makeDefaultModernPluginState() {
  ModernPluginState state;
  state.legacy_preset_bank = defaultLegacyFactoryBank();
  return state;
}

std::vector<std::byte> serializeModernPluginState(const ModernPluginState& state) {
  std::vector<std::byte> bytes;
  bytes.insert(bytes.end(), kModernStateMagic.begin(), kModernStateMagic.end());
  writePod(bytes, kModernStateVersion);

  const auto selected_program = state.selected_legacy_program_index.has_value()
                                    ? static_cast<std::uint32_t>(
                                          state.selected_legacy_program_index.value())
                                    : kNoSelectedProgram;
  writePod(bytes, selected_program);
  writeString(bytes, state.selected_legacy_program_name);

  writeLegacyState(bytes, state.state);

  const auto preset_count =
      static_cast<std::uint32_t>(state.legacy_preset_bank.presets.size());
  writePod(bytes, preset_count);
  for (const auto& preset : state.legacy_preset_bank.presets) {
    writePod(bytes, static_cast<std::uint32_t>(preset.program_index));
    writePod(bytes, preset.record_marker);
    writeString(bytes, preset.name);
    writeLegacyState(bytes, preset.state);
  }

  return bytes;
}

ModernStateDecodeResult deserializeModernPluginState(
    const std::span<const std::byte> bytes) {
  if (bytes.size() < kModernStateMagic.size() + sizeof(std::uint32_t)) {
    return {.error = ModernStateDecodeError{
                .offset = 0, .message = "State blob too small"}};
  }

  if (!std::equal(kModernStateMagic.begin(), kModernStateMagic.end(), bytes.begin())) {
    return {.error = ModernStateDecodeError{
                .offset = 0, .message = "Unexpected state blob magic"}};
  }

  std::size_t offset = kModernStateMagic.size();
  std::uint32_t version = 0;
  if (!readPod(bytes, offset, version)) {
    return {.error = ModernStateDecodeError{
                .offset = offset, .message = "Missing state blob version"}};
  }
  offset += sizeof(std::uint32_t);

  if (version != kModernStateVersion) {
    return {.error = ModernStateDecodeError{
                .offset = offset - sizeof(std::uint32_t),
                .message = "Unsupported state blob version"}};
  }

  ModernPluginState state;

  std::uint32_t selected_program = kNoSelectedProgram;
  if (!readPod(bytes, offset, selected_program)) {
    return {.error = ModernStateDecodeError{
                .offset = offset, .message = "Missing selected program index"}};
  }
  offset += sizeof(std::uint32_t);

  if (!readString(bytes, offset, state.selected_legacy_program_name)) {
    return {.error = ModernStateDecodeError{
                .offset = offset, .message = "Truncated selected program name"}};
  }

  if (!readLegacyState(bytes, offset, state.state)) {
    return {.error = ModernStateDecodeError{
                .offset = offset, .message = "Truncated current state block"}};
  }

  std::uint32_t preset_count = 0;
  if (!readPod(bytes, offset, preset_count)) {
    return {.error = ModernStateDecodeError{
                .offset = offset, .message = "Missing preset count"}};
  }
  offset += sizeof(std::uint32_t);

  state.legacy_preset_bank.presets.reserve(preset_count);
  for (std::uint32_t index = 0; index < preset_count; ++index) {
    LegacyPreset preset;
    std::uint32_t program_index = 0;
    if (!readPod(bytes, offset, program_index)) {
      return {.error = ModernStateDecodeError{
                  .offset = offset, .message = "Missing preset program index"}};
    }
    offset += sizeof(std::uint32_t);
    preset.program_index = program_index;

    if (!readPod(bytes, offset, preset.record_marker)) {
      return {.error = ModernStateDecodeError{
                  .offset = offset, .message = "Missing preset record marker"}};
    }
    offset += sizeof(float);

    if (!readString(bytes, offset, preset.name)) {
      return {.error = ModernStateDecodeError{
                  .offset = offset, .message = "Truncated preset name"}};
    }

    if (!readLegacyState(bytes, offset, preset.state)) {
      return {.error = ModernStateDecodeError{
                  .offset = offset, .message = "Truncated preset state block"}};
    }

    state.legacy_preset_bank.presets.push_back(std::move(preset));
  }

  if (selected_program != kNoSelectedProgram) {
    state.selected_legacy_program_index = selected_program;
    if (const auto* selected_preset =
            legacyPresetAt(state.legacy_preset_bank, selected_program);
        selected_preset != nullptr) {
      if (state.selected_legacy_program_name.empty()) {
        state.selected_legacy_program_name = selected_preset->name;
      }
      state.legacy_parameter_mismatches =
          buildParameterMismatches(state.state, selected_preset);
    }
  }

  return {.state = std::move(state)};
}

void ModernPluginRuntime::prepare(const ModernProcessSpec& spec) {
  processor_.prepare(spec);
}

void ModernPluginRuntime::reset() {
  processor_.reset();
}

const ModernPluginState& ModernPluginRuntime::state() const {
  return state_;
}

ModernPublicParameterArray ModernPluginRuntime::publicParameters() const {
  return modernPublicParameterArrayFromState(state_.state);
}

float ModernPluginRuntime::publicParameter(const std::size_t public_index) const {
  return modernPublicParameterValue(state_.state, public_index);
}

void ModernPluginRuntime::setPublicParameter(const std::size_t public_index,
                                             const float normalized_value) {
  setModernPublicParameterValue(state_.state, public_index, normalized_value);
  refreshSelectedProgramDiagnostics();
}

void ModernPluginRuntime::replaceState(const ModernPluginState& state) {
  state_ = state;
  refreshSelectedProgramDiagnostics();
  processor_.reset();
}

void ModernPluginRuntime::importLegacyState(const LegacyImportedState& imported_state) {
  state_ = modernPluginStateFromLegacyImport(imported_state);
  refreshSelectedProgramDiagnostics();
  processor_.reset();
}

void ModernPluginRuntime::clearSelectedLegacyPreset() {
  state_.selected_legacy_program_index.reset();
  state_.selected_legacy_program_name.clear();
  refreshSelectedProgramDiagnostics();
  processor_.reset();
}

bool ModernPluginRuntime::selectLegacyPreset(const std::size_t program_index,
                                             const bool adopt_preset_state) {
  const auto* preset = legacyPresetAt(state_.legacy_preset_bank, program_index);
  if (preset == nullptr) {
    return false;
  }

  state_.selected_legacy_program_index = program_index;
  state_.selected_legacy_program_name = preset->name;
  if (adopt_preset_state) {
    state_.state = preset->state;
  }
  refreshSelectedProgramDiagnostics();
  processor_.reset();
  return true;
}

std::vector<std::byte> ModernPluginRuntime::saveState() const {
  return serializeModernPluginState(state_);
}

ModernStateDecodeResult ModernPluginRuntime::loadState(
    const std::span<const std::byte> bytes) {
  auto result = deserializeModernPluginState(bytes);
  if (result.ok()) {
    replaceState(result.state.value());
  }
  return result;
}

void ModernPluginRuntime::processBlock(const ModernAudioBlock block) {
  processor_.processBlock(block, state_);
}

void ModernPluginRuntime::refreshSelectedProgramDiagnostics() {
  state_.legacy_parameter_mismatches.clear();

  if (!state_.selected_legacy_program_index.has_value()) {
    return;
  }

  const auto* preset = legacyPresetAt(
      state_.legacy_preset_bank, state_.selected_legacy_program_index.value());
  if (preset == nullptr) {
    return;
  }

  state_.selected_legacy_program_name = preset->name;
  state_.legacy_parameter_mismatches =
      buildParameterMismatches(state_.state, preset);
}

}  // namespace camelcrusher_recalled
