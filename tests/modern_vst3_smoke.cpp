#include "CamelCrusherVst3Controller.h"
#include "CamelCrusherVst3IDs.h"
#include "CamelCrusherVst3Processor.h"

#include "camelcrusher_recalled/ModernRuntime.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

using camelcrusher_recalled::ModernPluginState;
using camelcrusher_recalled::ModernPublicParameterArray;
using camelcrusher_recalled::makeDefaultModernPluginState;
using camelcrusher_recalled::modernPublicParameterArrayFromState;
using camelcrusher_recalled::serializeModernPluginState;
using camelcrusher_recalled::vst3::CamelCrusherVst3Controller;
using camelcrusher_recalled::vst3::CamelCrusherVst3Processor;
using camelcrusher_recalled::vst3::kCamelCrusherProgramParameterId;

bool nearlyEqual(const double left,
                 const double right,
                 const double epsilon = 0.000001) {
  return std::fabs(left - right) < epsilon;
}

std::string string128ToAscii(const Steinberg::Vst::String128& text) {
  char ascii[128]{};
  Steinberg::UString wrapper(const_cast<Steinberg::Vst::TChar*>(text),
                             USTRINGSIZE(text));
  wrapper.toAscii(ascii, 128);
  return ascii;
}

void writeWrappedState(Steinberg::MemoryStream& stream,
                       const std::vector<std::byte>& bytes) {
  Steinberg::IBStreamer streamer(&stream, kLittleEndian);
  const auto length = static_cast<Steinberg::uint32>(bytes.size());
  assert(streamer.writeInt32u(length));
  if (length > 0U) {
    assert(streamer.writeRaw(const_cast<std::byte*>(bytes.data()),
                             static_cast<Steinberg::int32>(bytes.size())));
  }
  Steinberg::int64 reset = 0;
  assert(stream.seek(0, Steinberg::IBStream::kIBSeekSet, &reset) ==
         Steinberg::kResultOk);
}

float peakMagnitude(const std::vector<float>& samples) {
  float peak = 0.0F;
  for (const auto sample : samples) {
    peak = std::max(peak, std::fabs(sample));
  }
  return peak;
}

}  // namespace

int main() {
  auto host = Steinberg::owned(new Steinberg::Vst::HostApplication());
  auto* processor = new CamelCrusherVst3Processor();
  auto* controller = new CamelCrusherVst3Controller();
  assert(processor != nullptr);
  assert(controller != nullptr);

  assert(processor->initialize(host.get()) == Steinberg::kResultOk);
  assert(controller->initialize(host.get()) == Steinberg::kResultOk);
  auto* view = controller->createView(Steinberg::Vst::ViewType::kEditor);
  assert(view != nullptr);
  assert(view->isPlatformTypeSupported(Steinberg::kPlatformTypeNSView) ==
         Steinberg::kResultTrue);
  view->release();

  assert(controller->getParameterCount() == 13);
  assert(controller->getProgramListCount() == 1);

  Steinberg::Vst::ProgramListInfo program_list_info{};
  assert(controller->getProgramListInfo(0, program_list_info) == Steinberg::kResultOk);
  assert(program_list_info.programCount == 21);
  assert(program_list_info.id == kCamelCrusherProgramParameterId);

  Steinberg::Vst::String128 program_name{};
  assert(controller->getProgramName(kCamelCrusherProgramParameterId, 0, program_name) ==
         Steinberg::kResultOk);
  assert(string128ToAscii(program_name) == "Current State");
  assert(controller->getProgramName(kCamelCrusherProgramParameterId, 1, program_name) ==
         Steinberg::kResultOk);
  assert(string128ToAscii(program_name) == "Annihilate");
  assert(controller->getProgramName(kCamelCrusherProgramParameterId, 20, program_name) ==
         Steinberg::kResultOk);
  assert(string128ToAscii(program_name) == "Turn it to 11");

  ModernPluginState imported = makeDefaultModernPluginState();
  imported.state.dist_on = 1.0F;
  imported.state.dist_mech = 0.82F;
  imported.state.dist_tube = 0.63F;
  imported.state.mm_filter_on = 1.0F;
  imported.state.mm_filter_cutoff = 0.24F;
  imported.state.mm_filter_res = 0.58F;
  imported.state.compress_on = 1.0F;
  imported.state.compress_amount = 0.71F;
  imported.state.compress_mode = 0.37F;
  imported.state.master_on = 1.0F;
  imported.state.master_mix = 0.96F;
  imported.state.master_volume = 0.61F;
  imported.selected_legacy_program_index = 3;
  imported.selected_legacy_program_name = "British Clean";

  const ModernPublicParameterArray expected_public =
      modernPublicParameterArrayFromState(imported.state);
  const auto state_bytes = serializeModernPluginState(imported);

  Steinberg::MemoryStream import_stream;
  writeWrappedState(import_stream, state_bytes);
  assert(processor->setState(&import_stream) == Steinberg::kResultOk);

  Steinberg::MemoryStream controller_stream;
  writeWrappedState(controller_stream, state_bytes);
  assert(controller->setComponentState(&controller_stream) == Steinberg::kResultOk);

  for (Steinberg::int32 index = 0; index < 12; ++index) {
    assert(nearlyEqual(controller->getParamNormalized(index), expected_public[index]));
  }

  const auto selected_program = controller->getParamNormalized(
      kCamelCrusherProgramParameterId);
  assert(selected_program > 0.0);

  Steinberg::Vst::ProcessSetup setup{};
  setup.processMode = Steinberg::Vst::kRealtime;
  setup.symbolicSampleSize = Steinberg::Vst::kSample32;
  setup.maxSamplesPerBlock = 16;
  setup.sampleRate = 48000.0;
  assert(processor->setupProcessing(setup) == Steinberg::kResultOk);
  assert(processor->setActive(true) == Steinberg::kResultOk);

  std::vector<float> input_left(16, 0.0F);
  std::vector<float> input_right(16, 0.0F);
  input_left[0] = 0.7F;
  input_right[0] = 0.7F;

  std::vector<float> output_left = input_left;
  std::vector<float> output_right = input_right;

  float* input_channels[2] = {input_left.data(), input_right.data()};
  float* output_channels[2] = {output_left.data(), output_right.data()};

  Steinberg::Vst::AudioBusBuffers inputs{};
  inputs.numChannels = 2;
  inputs.silenceFlags = 0;
  inputs.channelBuffers32 = input_channels;

  Steinberg::Vst::AudioBusBuffers outputs{};
  outputs.numChannels = 2;
  outputs.silenceFlags = 0;
  outputs.channelBuffers32 = output_channels;

  Steinberg::Vst::ProcessData process_data{};
  process_data.processMode = Steinberg::Vst::kRealtime;
  process_data.symbolicSampleSize = Steinberg::Vst::kSample32;
  process_data.numSamples = 16;
  process_data.numInputs = 1;
  process_data.numOutputs = 1;
  process_data.inputs = &inputs;
  process_data.outputs = &outputs;

  assert(processor->process(process_data) == Steinberg::kResultOk);
  assert(!nearlyEqual(output_left[0], 0.7F));
  assert(!nearlyEqual(output_right[0], 0.7F));
  assert(peakMagnitude(output_left) <= 1.0F);
  assert(peakMagnitude(output_right) <= 1.0F);

  ModernPluginState dry_state = makeDefaultModernPluginState();
  dry_state.state.master_on = 1.0F;
  dry_state.state.master_mix = 1.0F;
  dry_state.state.master_volume = 0.5F;
  const auto dry_state_bytes = serializeModernPluginState(dry_state);
  Steinberg::MemoryStream dry_stream;
  writeWrappedState(dry_stream, dry_state_bytes);
  assert(processor->setState(&dry_stream) == Steinberg::kResultOk);
  assert(processor->setActive(false) == Steinberg::kResultOk);
  assert(processor->setActive(true) == Steinberg::kResultOk);

  std::vector<float> right_only_left(8, 0.0F);
  std::vector<float> right_only_right(8, 0.0F);
  right_only_right[0] = 0.65F;
  std::vector<float> right_only_out_left = right_only_left;
  std::vector<float> right_only_out_right = right_only_right;

  float* right_only_input_channels[2] = {
      right_only_left.data(),
      right_only_right.data(),
  };
  float* right_only_output_channels[2] = {
      right_only_out_left.data(),
      right_only_out_right.data(),
  };

  Steinberg::Vst::AudioBusBuffers right_only_inputs{};
  right_only_inputs.numChannels = 2;
  right_only_inputs.silenceFlags = 0;
  right_only_inputs.channelBuffers32 = right_only_input_channels;

  Steinberg::Vst::AudioBusBuffers right_only_outputs{};
  right_only_outputs.numChannels = 2;
  right_only_outputs.silenceFlags = 0;
  right_only_outputs.channelBuffers32 = right_only_output_channels;

  Steinberg::Vst::ProcessData right_only_process{};
  right_only_process.processMode = Steinberg::Vst::kRealtime;
  right_only_process.symbolicSampleSize = Steinberg::Vst::kSample32;
  right_only_process.numSamples = 8;
  right_only_process.numInputs = 1;
  right_only_process.numOutputs = 1;
  right_only_process.inputs = &right_only_inputs;
  right_only_process.outputs = &right_only_outputs;

  assert(processor->process(right_only_process) == Steinberg::kResultOk);
  assert(nearlyEqual(right_only_out_left[0], 0.0, 0.000001));
  assert(nearlyEqual(right_only_out_right[0], 0.65, 0.000001));

  std::vector<float> mono_input(8, 0.0F);
  mono_input[0] = 0.37F;
  std::vector<float> mono_out_left(8, 0.0F);
  std::vector<float> mono_out_right(8, 0.0F);
  float* mono_input_channels[1] = {mono_input.data()};
  float* mono_output_channels[2] = {
      mono_out_left.data(),
      mono_out_right.data(),
  };

  Steinberg::Vst::AudioBusBuffers mono_inputs{};
  mono_inputs.numChannels = 1;
  mono_inputs.silenceFlags = 0;
  mono_inputs.channelBuffers32 = mono_input_channels;

  Steinberg::Vst::AudioBusBuffers mono_outputs{};
  mono_outputs.numChannels = 2;
  mono_outputs.silenceFlags = 0;
  mono_outputs.channelBuffers32 = mono_output_channels;

  Steinberg::Vst::ProcessData mono_process{};
  mono_process.processMode = Steinberg::Vst::kRealtime;
  mono_process.symbolicSampleSize = Steinberg::Vst::kSample32;
  mono_process.numSamples = 8;
  mono_process.numInputs = 1;
  mono_process.numOutputs = 1;
  mono_process.inputs = &mono_inputs;
  mono_process.outputs = &mono_outputs;

  assert(processor->process(mono_process) == Steinberg::kResultOk);
  assert(nearlyEqual(mono_out_left[0], 0.37, 0.000001));
  assert(nearlyEqual(mono_out_right[0], 0.37, 0.000001));

  Steinberg::MemoryStream saved_state;
  assert(processor->getState(&saved_state) == Steinberg::kResultOk);
  Steinberg::int64 reset = 0;
  assert(saved_state.seek(0, Steinberg::IBStream::kIBSeekSet, &reset) ==
         Steinberg::kResultOk);
  assert(controller->setComponentState(&saved_state) == Steinberg::kResultOk);
  const auto expected_dry_public =
      modernPublicParameterArrayFromState(dry_state.state);
  for (Steinberg::int32 index = 0; index < 12; ++index) {
    assert(nearlyEqual(controller->getParamNormalized(index),
                       expected_dry_public[index]));
  }

  processor->setActive(false);
  processor->terminate();
  controller->terminate();
  processor->release();
  controller->release();
  return 0;
}
