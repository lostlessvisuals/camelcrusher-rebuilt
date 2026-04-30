#include "CamelCrusherVst3State.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/base/ustring.h"

#include <algorithm>
#include <string>

namespace camelcrusher_recalled::vst3 {

const ModernPublicParameterDescriptor* publicDescriptorForParamId(
    const Steinberg::Vst::ParamID param_id) {
  const auto descriptors = modernPublicParameterDescriptors();
  if (param_id < 0 ||
      static_cast<std::size_t>(param_id) >= descriptors.size()) {
    return nullptr;
  }
  return &descriptors[static_cast<std::size_t>(param_id)];
}

Steinberg::int32 programSlotCount(const ModernPluginState& state) {
  return static_cast<Steinberg::int32>(state.legacy_preset_bank.presets.size() + 1U);
}

std::optional<Steinberg::int32> selectedProgramSlot(
    const ModernPluginState& state) {
  if (!state.selected_legacy_program_index.has_value()) {
    return kCamelCrusherCurrentStateProgramSlot;
  }

  return static_cast<Steinberg::int32>(
      state.selected_legacy_program_index.value() + 1U);
}

std::optional<std::size_t> legacyPresetIndexForProgramSlot(
    const ModernPluginState& state,
    const Steinberg::int32 program_slot) {
  if (program_slot <= kCamelCrusherCurrentStateProgramSlot) {
    return std::nullopt;
  }

  const auto preset_index =
      static_cast<std::size_t>(program_slot - 1);
  if (preset_index >= state.legacy_preset_bank.presets.size()) {
    return std::nullopt;
  }
  return preset_index;
}

Steinberg::Vst::ParamValue normalizedProgramSlot(
    const Steinberg::int32 program_slot,
    const Steinberg::int32 program_slot_count) {
  const auto max_index = std::max(program_slot_count - 1, 0);
  if (max_index == 0) {
    return 0.0;
  }
  return Steinberg::ToNormalized<Steinberg::Vst::ParamValue>(
      std::clamp(program_slot, 0, max_index),
      max_index);
}

Steinberg::int32 plainProgramSlot(
    const Steinberg::Vst::ParamValue normalized_value,
    const Steinberg::int32 program_slot_count) {
  const auto max_index = std::max(program_slot_count - 1, 0);
  if (max_index == 0) {
    return 0;
  }
  return Steinberg::FromNormalized<Steinberg::Vst::ParamValue>(
      normalized_value,
      max_index);
}

void assignString128(Steinberg::Vst::String128& target, const char* ascii_text) {
  Steinberg::UString(target, USTRINGSIZE(target))
      .fromAscii(ascii_text != nullptr ? ascii_text : "");
}

void populateProgramList(Steinberg::Vst::ProgramList& list,
                         const ModernPluginState& state) {
  list.clearPrograms();
  list.addProgram(STR("Current State"));
  for (const auto& preset : state.legacy_preset_bank.presets) {
    std::string ascii_name = preset.name;
    if (ascii_name.empty()) {
      ascii_name = "Preset";
    }
    Steinberg::Vst::String128 text{};
    assignString128(text, ascii_name.c_str());
    list.addProgram(text);
  }
}

Steinberg::tresult writeRuntimeStateBlob(
    Steinberg::IBStream* stream,
    const std::span<const std::byte> bytes) {
  if (stream == nullptr) {
    return Steinberg::kResultFalse;
  }

  Steinberg::IBStreamer streamer(stream, kLittleEndian);
  const auto byte_count = static_cast<Steinberg::uint32>(bytes.size());
  if (!streamer.writeInt32u(byte_count)) {
    return Steinberg::kResultFalse;
  }

  if (byte_count == 0U) {
    return Steinberg::kResultOk;
  }

  return streamer.writeRaw(
             const_cast<std::byte*>(bytes.data()),
             static_cast<Steinberg::int32>(bytes.size()))
             ? Steinberg::kResultOk
             : Steinberg::kResultFalse;
}

Steinberg::tresult readRuntimeStateBlob(
    Steinberg::IBStream* stream,
    std::vector<std::byte>& out_bytes) {
  if (stream == nullptr) {
    return Steinberg::kResultFalse;
  }

  Steinberg::IBStreamer streamer(stream, kLittleEndian);
  Steinberg::uint32 byte_count = 0;
  if (!streamer.readInt32u(byte_count)) {
    return Steinberg::kResultFalse;
  }

  out_bytes.resize(byte_count);
  if (byte_count == 0U) {
    return Steinberg::kResultOk;
  }

  return streamer.readRaw(
             out_bytes.data(),
             static_cast<Steinberg::int32>(byte_count))
             ? Steinberg::kResultOk
             : Steinberg::kResultFalse;
}

}  // namespace camelcrusher_recalled::vst3
