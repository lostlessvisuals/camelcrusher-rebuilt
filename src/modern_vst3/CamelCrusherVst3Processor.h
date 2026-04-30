#pragma once

#include "camelcrusher_recalled/ModernRuntime.h"
#include "public.sdk/source/vst/vstaudioeffect.h"

namespace camelcrusher_recalled::vst3 {

class CamelCrusherVst3Processor : public Steinberg::Vst::AudioEffect {
 public:
  CamelCrusherVst3Processor();

  static Steinberg::FUnknown* createInstance(void*) {
    return static_cast<Steinberg::Vst::IAudioProcessor*>(
        new CamelCrusherVst3Processor());
  }

  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setupProcessing(
      Steinberg::Vst::ProcessSetup& new_setup) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setBusArrangements(
      Steinberg::Vst::SpeakerArrangement* inputs,
      Steinberg::int32 num_inputs,
      Steinberg::Vst::SpeakerArrangement* outputs,
      Steinberg::int32 num_outputs) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(
      Steinberg::int32 symbolic_sample_size) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API process(
      Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

 private:
  void applyParameterChanges(
      Steinberg::Vst::IParameterChanges* parameter_changes);
  void clearOutputs(Steinberg::Vst::ProcessData& data) const;

  ModernPluginRuntime runtime_;
  std::size_t channel_count_ = 2;
};

}  // namespace camelcrusher_recalled::vst3
