#pragma once

#include "StereoCases.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"

inline void checkVst3Stereo(Steinberg::Vst::IComponent* component) {
  using namespace Steinberg;
  using namespace Steinberg::Vst;
  auto host = owned(new HostApplication());
  CC_CHECK(component->initialize(host.get()) == kResultOk);
  FUnknownPtr<IAudioProcessor> processor(component);
  CC_CHECK(processor != nullptr);
  CC_CHECK(component->activateBus(kAudio, kInput, 0, true) == kResultOk);
  CC_CHECK(component->activateBus(kAudio, kOutput, 0, true) == kResultOk);
  int cases = 0;
  for (int layout = 0; layout < 3; ++layout) {
    const int input_count = layout == 0 ? 2 : 1;
    const int output_count = layout == 2 ? 1 : 2;
    auto in_arr = input_count == 1 ? SpeakerArr::kMono : SpeakerArr::kStereo;
    auto out_arr = output_count == 1 ? SpeakerArr::kMono : SpeakerArr::kStereo;
    CC_CHECK(processor->setBusArrangements(&in_arr, 1, &out_arr, 1) == kResultOk);
    SpeakerArrangement queried{};
    CC_CHECK(processor->getBusArrangement(kInput, 0, queried) == kResultOk && queried == in_arr);
    CC_CHECK(processor->getBusArrangement(kOutput, 0, queried) == kResultOk && queried == out_arr);
    for (double rate : {44100.0, 48000.0, 96000.0}) {
      ProcessSetup setup{kRealtime, kSample32, 1024, rate};
      CC_CHECK(processor->setupProcessing(setup) == kResultOk);
      CC_CHECK(component->setActive(true) == kResultOk);
      CC_CHECK(processor->setProcessing(true) == kResultOk);
      for (int size : {1, 128, 257, 512, 1024}) {
        for (int pattern = 0; pattern < 6; ++pattern) {
          for (bool inplace : {false, true}) {
            StereoCase signal(size, pattern);
            auto left = signal.left, right = signal.right;
            std::vector<float> ol(size), oright(size);
            float* in[] = {left.data(), right.data()};
            float* out[] = {inplace ? left.data() : ol.data(), inplace ? right.data() : oright.data()};
            AudioBusBuffers input{}, output{};
            input.numChannels = input_count;
            output.numChannels = output_count;
            input.channelBuffers32 = in;
            output.channelBuffers32 = out;
            if (pattern == 5) input.silenceFlags = input_count == 2 ? 3 : 1;
            ParameterChanges changes;
            int32 index = 0;
            changes.addParameterData(9, index)->addPoint(0, 0.0, index);
            ProcessData data{};
            data.processMode = kRealtime;
            data.symbolicSampleSize = kSample32;
            data.numSamples = size;
            data.numInputs = data.numOutputs = 1;
            data.inputs = &input;
            data.outputs = &output;
            data.inputParameterChanges = &changes;
            CC_CHECK(processor->process(data) == kResultOk);
            for (int i = 0; i < size; ++i) {
              checkSample(out[0][i], signal.left[i]);
              if (output_count == 2) checkSample(out[1][i], input_count == 1 ? signal.left[i] : signal.right[i]);
            }
            ++cases;
          }
        }
      }
      CC_CHECK(processor->setProcessing(false) == kResultOk);
      CC_CHECK(component->setActive(false) == kResultOk);
    }
  }

  auto stereo = SpeakerArr::kStereo;
  CC_CHECK(processor->setBusArrangements(&stereo, 1, &stereo, 1) == kResultOk);
  ProcessSetup setup{kRealtime, kSample32, 512, 48000.0};
  CC_CHECK(processor->setupProcessing(setup) == kResultOk);
  CC_CHECK(component->setActive(true) == kResultOk);
  CC_CHECK(processor->setProcessing(true) == kResultOk);
  StereoCase impulse(512, 5);
  impulse.right.back() = 0.5F;
  std::vector<float> left(512), right(512);
  float* in[] = {impulse.left.data(), impulse.right.data()};
  float* out[] = {left.data(), right.data()};
  AudioBusBuffers input{}, output{};
  input.numChannels = output.numChannels = 2;
  input.channelBuffers32 = in;
  output.channelBuffers32 = out;
  ParameterChanges changes;
  const double params[] = {0, 0, 0, 1, 0.2, 0.3, 0, 0, 0, 1, 1, 0.5};
  for (int32 p = 0; p < 12; ++p) {
    int32 index = 0;
    changes.addParameterData(p, index)->addPoint(0, params[p], index);
  }
  ProcessData data{};
  data.processMode = kRealtime;
  data.symbolicSampleSize = kSample32;
  data.numSamples = 512;
  data.numInputs = data.numOutputs = 1;
  data.inputs = &input;
  data.outputs = &output;
  data.inputParameterChanges = &changes;
  CC_CHECK(processor->process(data) == kResultOk);
  input.silenceFlags = 3;
  data.inputParameterChanges = nullptr;
  CC_CHECK(processor->process(data) == kResultOk);
  CC_CHECK(std::any_of(right.begin(), right.end(), [](float v) { return std::abs(v) > 1.0e-6F; }));
  CC_CHECK(std::all_of(left.begin(), left.end(), [](float v) { return v == 0.0F; }));
  CC_CHECK(processor->setProcessing(false) == kResultOk);
  CC_CHECK(component->setActive(false) == kResultOk);
  CC_CHECK(component->terminate() == kResultOk);
  std::cout << cases << " VST3 routing cases and silent-input filter tail passed\n";
}
