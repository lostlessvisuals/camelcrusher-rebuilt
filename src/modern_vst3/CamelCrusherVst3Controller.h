#pragma once

#include "CamelCrusherVst3Editor.h"
#include "CamelCrusherVst3State.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace camelcrusher_recalled::vst3 {

class CamelCrusherVst3Controller : public Steinberg::Vst::EditControllerEx1 {
 public:
  static Steinberg::FUnknown* createInstance(void*) {
    return static_cast<Steinberg::Vst::IEditController*>(
        new CamelCrusherVst3Controller());
  }

  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setComponentState(
      Steinberg::IBStream* state) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;
  Steinberg::tresult PLUGIN_API setParamNormalized(
      Steinberg::Vst::ParamID tag,
      Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
  Steinberg::IPlugView* PLUGIN_API createView(const char* name) SMTG_OVERRIDE;

  void setEditorObserver(CamelCrusherVst3EditorObserver* observer);
  ModernPublicParameterArray publicParameterValues() const;
  Steinberg::int32 currentProgramSlotValue() const;
  std::vector<std::string> currentProgramNames() const;
  void applyViewParameterEdit(Steinberg::Vst::ParamID tag,
                              Steinberg::Vst::ParamValue value);

  void editorDestroyed(Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;

 private:
  void rebuildProgramList();
  void syncPublicParametersFromState();
  void syncProgramParameterFromState();
  void applySelectedProgram(std::size_t preset_index);
  Steinberg::tresult loadWrappedState(Steinberg::IBStream* state);
  void notifyEditorObserver();

  ModernPluginState state_ = makeDefaultModernPluginState();
  CamelCrusherVst3EditorObserver* editor_observer_ = nullptr;
  bool syncing_program_parameter_ = false;
};

}  // namespace camelcrusher_recalled::vst3
