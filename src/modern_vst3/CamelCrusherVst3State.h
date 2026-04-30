#pragma once

#include "CamelCrusherVst3IDs.h"
#include "camelcrusher_recalled/ModernRuntime.h"

#include "pluginterfaces/base/ibstream.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace camelcrusher_recalled::vst3 {

const ModernPublicParameterDescriptor* publicDescriptorForParamId(
    Steinberg::Vst::ParamID param_id);

Steinberg::int32 programSlotCount(const ModernPluginState& state);
std::optional<Steinberg::int32> selectedProgramSlot(
    const ModernPluginState& state);
std::optional<std::size_t> legacyPresetIndexForProgramSlot(
    const ModernPluginState& state,
    Steinberg::int32 program_slot);

Steinberg::Vst::ParamValue normalizedProgramSlot(
    Steinberg::int32 program_slot,
    Steinberg::int32 program_slot_count);
Steinberg::int32 plainProgramSlot(
    Steinberg::Vst::ParamValue normalized_value,
    Steinberg::int32 program_slot_count);

void assignString128(Steinberg::Vst::String128& target, const char* ascii_text);
void populateProgramList(Steinberg::Vst::ProgramList& list,
                         const ModernPluginState& state);

Steinberg::tresult writeRuntimeStateBlob(
    Steinberg::IBStream* stream,
    std::span<const std::byte> bytes);
Steinberg::tresult readRuntimeStateBlob(
    Steinberg::IBStream* stream,
    std::vector<std::byte>& out_bytes);

}  // namespace camelcrusher_recalled::vst3
