#include "CamelCrusherVst3Controller.h"

#include "CamelCrusherVst3Editor.h"

#include "pluginterfaces/base/ustring.h"

#include <cstring>
#include <string>
#include <vector>

namespace camelcrusher_recalled::vst3 {
namespace {

Steinberg::int32 parameterFlagsForDescriptor(
    const ModernPublicParameterDescriptor&) {
  return Steinberg::Vst::ParameterInfo::kCanAutomate;
}

Steinberg::int32 stepCountForDescriptor(
    const ModernPublicParameterDescriptor& descriptor) {
  return descriptor.value_kind == LegacyParameterValueKind::kSwitchLike ? 1 : 0;
}

}  // namespace

Steinberg::tresult PLUGIN_API CamelCrusherVst3Controller::initialize(
    Steinberg::FUnknown* context) {
  const auto result = EditControllerEx1::initialize(context);
  if (result != Steinberg::kResultOk) {
    return result;
  }

  const auto defaults = modernPublicParameterArrayFromState(state_.state);
  for (const auto& descriptor : modernPublicParameterDescriptors()) {
    std::string title(descriptor.display_name);
    Steinberg::Vst::String128 title_text{};
    assignString128(title_text, title.c_str());
    parameters.addParameter(
        new Steinberg::Vst::Parameter(
            title_text,
            static_cast<Steinberg::Vst::ParamID>(descriptor.public_index),
            nullptr,
            defaults[descriptor.public_index],
            stepCountForDescriptor(descriptor),
            parameterFlagsForDescriptor(descriptor)));
  }

  addUnit(new Steinberg::Vst::Unit(
      STR("Root"),
      Steinberg::Vst::kRootUnitId,
      Steinberg::Vst::kNoParentUnitId,
      kCamelCrusherProgramParameterId));

  auto* program_list = new Steinberg::Vst::ProgramList(
      STR("Factory Preset"),
      kCamelCrusherProgramParameterId,
      Steinberg::Vst::kRootUnitId);
  addProgramList(program_list);
  populateProgramList(*program_list, state_);

  auto* program_parameter = program_list->getParameter();
  program_parameter->getInfo().flags &= ~Steinberg::Vst::ParameterInfo::kCanAutomate;
  parameters.addParameter(program_parameter);

  rebuildProgramList();
  syncPublicParametersFromState();
  syncProgramParameterFromState();
  return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Controller::setComponentState(
    Steinberg::IBStream* state) {
  return loadWrappedState(state);
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Controller::setState(
    Steinberg::IBStream* state) {
  return loadWrappedState(state);
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Controller::getState(
    Steinberg::IBStream* state) {
  const auto bytes = serializeModernPluginState(state_);
  return writeRuntimeStateBlob(state, bytes);
}

Steinberg::IPlugView* PLUGIN_API CamelCrusherVst3Controller::createView(
    const char* name) {
  if (name == nullptr ||
      std::strcmp(name, Steinberg::Vst::ViewType::kEditor) != 0) {
    return nullptr;
  }

  return createCamelCrusherVst3Editor(*this);
}

void CamelCrusherVst3Controller::setEditorObserver(
    CamelCrusherVst3EditorObserver* observer) {
  editor_observer_ = observer;
}

ModernPublicParameterArray CamelCrusherVst3Controller::publicParameterValues()
    const {
  return modernPublicParameterArrayFromState(state_.state);
}

Steinberg::int32 CamelCrusherVst3Controller::currentProgramSlotValue() const {
  return selectedProgramSlot(state_).value_or(
      kCamelCrusherCurrentStateProgramSlot);
}

std::vector<std::string> CamelCrusherVst3Controller::currentProgramNames()
    const {
  std::vector<std::string> names;
  names.emplace_back("Current State");
  for (const auto& preset : state_.legacy_preset_bank.presets) {
    names.push_back(preset.name);
  }
  return names;
}

void CamelCrusherVst3Controller::applyViewParameterEdit(
    const Steinberg::Vst::ParamID tag,
    const Steinberg::Vst::ParamValue value) {
  beginEdit(tag);
  setParamNormalized(tag, value);
  performEdit(tag, getParamNormalized(tag));
  endEdit(tag);
}

Steinberg::tresult PLUGIN_API CamelCrusherVst3Controller::setParamNormalized(
    const Steinberg::Vst::ParamID tag,
    const Steinberg::Vst::ParamValue value) {
  const auto result = EditControllerEx1::setParamNormalized(tag, value);
  if (result != Steinberg::kResultOk) {
    return result;
  }

  if (tag == kCamelCrusherProgramParameterId) {
    if (syncing_program_parameter_) {
      return Steinberg::kResultOk;
    }

    const auto slot = plainProgramSlot(value, programSlotCount(state_));
    if (slot == kCamelCrusherCurrentStateProgramSlot) {
      state_.selected_legacy_program_index.reset();
      state_.selected_legacy_program_name.clear();
      state_.legacy_parameter_mismatches.clear();
      notifyEditorObserver();
      return Steinberg::kResultOk;
    }

    if (const auto preset_index = legacyPresetIndexForProgramSlot(state_, slot);
        preset_index.has_value()) {
      applySelectedProgram(preset_index.value());
      syncPublicParametersFromState();
      notifyEditorObserver();
    }
    return Steinberg::kResultOk;
  }

  if (const auto* descriptor = publicDescriptorForParamId(tag);
      descriptor != nullptr) {
    setModernPublicParameterValue(
        state_.state,
        descriptor->public_index,
        static_cast<float>(value));
    notifyEditorObserver();
  }

  return Steinberg::kResultOk;
}

void CamelCrusherVst3Controller::rebuildProgramList() {
  auto* list = getProgramList(kCamelCrusherProgramParameterId);
  if (list == nullptr) {
    return;
  }

  populateProgramList(*list, state_);
  if (auto* parameter = list->getParameter(); parameter != nullptr) {
    parameter->getInfo().stepCount = std::max(programSlotCount(state_) - 1, 0);
    parameter->getInfo().defaultNormalizedValue =
        normalizedProgramSlot(
            kCamelCrusherCurrentStateProgramSlot,
            programSlotCount(state_));
  }
}

void CamelCrusherVst3Controller::syncPublicParametersFromState() {
  const auto values = modernPublicParameterArrayFromState(state_.state);
  for (const auto& descriptor : modernPublicParameterDescriptors()) {
    EditControllerEx1::setParamNormalized(
        static_cast<Steinberg::Vst::ParamID>(descriptor.public_index),
        values[descriptor.public_index]);
  }
}

void CamelCrusherVst3Controller::syncProgramParameterFromState() {
  syncing_program_parameter_ = true;
  const auto slot = selectedProgramSlot(state_).value_or(
      kCamelCrusherCurrentStateProgramSlot);
  EditControllerEx1::setParamNormalized(
      kCamelCrusherProgramParameterId,
      normalizedProgramSlot(slot, programSlotCount(state_)));
  syncing_program_parameter_ = false;
}

void CamelCrusherVst3Controller::applySelectedProgram(
    const std::size_t preset_index) {
  const auto* preset =
      legacyPresetAt(state_.legacy_preset_bank, preset_index);
  if (preset == nullptr) {
    return;
  }

  state_.selected_legacy_program_index = preset_index;
  state_.selected_legacy_program_name = preset->name;
  state_.legacy_parameter_mismatches.clear();
  state_.state = preset->state;
}

Steinberg::tresult CamelCrusherVst3Controller::loadWrappedState(
    Steinberg::IBStream* state) {
  std::vector<std::byte> bytes;
  if (readRuntimeStateBlob(state, bytes) != Steinberg::kResultOk) {
    return Steinberg::kResultFalse;
  }

  const auto decoded = deserializeModernPluginState(bytes);
  if (!decoded.ok()) {
    return Steinberg::kResultFalse;
  }

  state_ = decoded.state.value();
  rebuildProgramList();
  syncPublicParametersFromState();
  syncProgramParameterFromState();
  notifyEditorObserver();
  return Steinberg::kResultOk;
}

void CamelCrusherVst3Controller::editorDestroyed(
    Steinberg::Vst::EditorView* editor) {
  editor_observer_ = nullptr;
  EditControllerEx1::editorDestroyed(editor);
}

void CamelCrusherVst3Controller::notifyEditorObserver() {
  if (editor_observer_ != nullptr) {
    editor_observer_->vst3ControllerDidChangeState();
  }
}

}  // namespace camelcrusher_recalled::vst3
