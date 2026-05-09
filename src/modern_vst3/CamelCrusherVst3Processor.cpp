#include "CamelCrusherVst3Processor.h"

#include "CamelCrusherVst3IDs.h"
#include "CamelCrusherVst3State.h"

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace camelcrusher_recalled::vst3 {
CamelCrusherVst3Processor::CamelCrusherVst3Processor() {
  setControllerClass(kCamelCrusherVst3ControllerUID);
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Processor::initialize(
    Steinberg::FUnknown* context) {
  const auto result = AudioEffect::initialize(context);
  if (result != Steinberg::kResultOk) {
    return result;
  }

  addAudioInput(STR16("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
  addAudioOutput(STR16("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);
  return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Processor::setActive(
    const Steinberg::TBool state) {
  runtime_.reset();
  return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Processor::setupProcessing(
    Steinberg::Vst::ProcessSetup& new_setup) {
  const auto result = AudioEffect::setupProcessing(new_setup);
  if (result != Steinberg::kResultOk) {
    return result;
  }

  runtime_.prepare(ModernProcessSpec{
      .sample_rate = new_setup.sampleRate,
      .max_block_size = static_cast<std::size_t>(new_setup.maxSamplesPerBlock),
      .channel_count = channel_count_,
  });
  return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Processor::setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs,
    const Steinberg::int32 num_inputs,
    Steinberg::Vst::SpeakerArrangement* outputs,
    const Steinberg::int32 num_outputs) {
  if (num_inputs != 1 || num_outputs != 1) {
    return Steinberg::kResultFalse;
  }

  if (inputs[0] != outputs[0]) {
    return Steinberg::kResultFalse;
  }

  if (inputs[0] != Steinberg::Vst::SpeakerArr::kMono &&
      inputs[0] != Steinberg::Vst::SpeakerArr::kStereo) {
    return Steinberg::kResultFalse;
  }

  channel_count_ =
      inputs[0] == Steinberg::Vst::SpeakerArr::kMono ? 1U : 2U;
  return AudioEffect::setBusArrangements(inputs, num_inputs, outputs, num_outputs);
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Processor::canProcessSampleSize(
    const Steinberg::int32 symbolic_sample_size) {
  return symbolic_sample_size == Steinberg::Vst::kSample32
             ? Steinberg::kResultOk
             : Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Processor::process(
    Steinberg::Vst::ProcessData& data) {
  if (data.symbolicSampleSize != Steinberg::Vst::kSample32) {
    return Steinberg::kResultFalse;
  }

  applyParameterChanges(data.inputParameterChanges);

  if (data.numInputs == 0 || data.numOutputs == 0) {
    return Steinberg::kResultOk;
  }

  auto& input = data.inputs[0];
  auto& output = data.outputs[0];

  if (input.numChannels == 0 || output.numChannels == 0) {
    return Steinberg::kResultOk;
  }

  const auto silence_mask =
      Steinberg::Vst::getChannelMask(input.numChannels);
  if (input.silenceFlags == silence_mask) {
    output.silenceFlags = silence_mask;
    clearOutputs(data);
    return Steinberg::kResultOk;
  }

  const auto sample_count = static_cast<std::size_t>(data.numSamples);

  if (input.channelBuffers32 == nullptr || output.channelBuffers32 == nullptr ||
      input.channelBuffers32[0] == nullptr ||
      output.channelBuffers32[0] == nullptr) {
    return Steinberg::kResultFalse;
  }

  auto* left_out = output.channelBuffers32[0];
  if (input.channelBuffers32[0] != left_out) {
    std::memcpy(left_out, input.channelBuffers32[0], sample_count * sizeof(float));
  }

  std::span<float> left(left_out, sample_count);
  std::span<float> right;

  const auto has_stereo_output =
      output.numChannels > 1 && output.channelBuffers32[1] != nullptr;
  const auto has_stereo_input =
      input.numChannels > 1 && input.channelBuffers32[1] != nullptr;

  if (has_stereo_output) {
    auto* right_out = output.channelBuffers32[1];
    const auto* right_in =
        has_stereo_input ? input.channelBuffers32[1] : input.channelBuffers32[0];
    if (right_in != right_out) {
      std::memcpy(right_out, right_in, sample_count * sizeof(float));
    }
    right = std::span<float>(right_out, sample_count);
  }

  runtime_.processBlock(ModernAudioBlock{
      .left = left,
      .right = right,
  });
  output.silenceFlags = 0;
  return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Processor::setState(
    Steinberg::IBStream* state) {
  std::vector<std::byte> bytes;
  if (readRuntimeStateBlob(state, bytes) != Steinberg::kResultOk) {
    return Steinberg::kResultFalse;
  }

  const auto result = runtime_.loadState(bytes);
  return result.ok() ? Steinberg::kResultOk : Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Processor::getState(
    Steinberg::IBStream* state) {
  const auto bytes = runtime_.saveState();
  return writeRuntimeStateBlob(state, bytes);
}

void CamelCrusherVst3Processor::applyParameterChanges(
    Steinberg::Vst::IParameterChanges* parameter_changes) {
  if (parameter_changes == nullptr) {
    return;
  }

  const auto count = parameter_changes->getParameterCount();
  const auto descriptors = modernPublicParameterDescriptors();
  for (Steinberg::int32 index = 0; index < count; ++index) {
    auto* queue = parameter_changes->getParameterData(index);
    if (queue == nullptr || queue->getPointCount() <= 0) {
      continue;
    }

    Steinberg::Vst::ParamValue value = 0.0;
    Steinberg::int32 sample_offset = 0;
    if (queue->getPoint(queue->getPointCount() - 1, sample_offset, value) !=
        Steinberg::kResultTrue) {
      continue;
    }

    const auto parameter_id = queue->getParameterId();
    if (parameter_id >= 0 &&
        static_cast<std::size_t>(parameter_id) < descriptors.size()) {
      runtime_.setPublicParameter(
          static_cast<std::size_t>(parameter_id),
          static_cast<float>(value));
      continue;
    }

    if (parameter_id != kCamelCrusherProgramParameterId) {
      continue;
    }

    const auto slot =
        plainProgramSlot(value, programSlotCount(runtime_.state()));
    if (slot == kCamelCrusherCurrentStateProgramSlot) {
      runtime_.clearSelectedLegacyPreset();
      continue;
    }

    if (const auto preset_index =
            legacyPresetIndexForProgramSlot(runtime_.state(), slot);
        preset_index.has_value()) {
      runtime_.selectLegacyPreset(preset_index.value(), true);
    }
  }
}

void CamelCrusherVst3Processor::clearOutputs(
    Steinberg::Vst::ProcessData& data) const {
  auto& output = data.outputs[0];
  const auto sample_count = static_cast<std::size_t>(data.numSamples);
  for (Steinberg::int32 channel = 0; channel < output.numChannels; ++channel) {
    std::memset(output.channelBuffers32[channel], 0, sample_count * sizeof(float));
  }
}

}  // namespace camelcrusher_recalled::vst3
