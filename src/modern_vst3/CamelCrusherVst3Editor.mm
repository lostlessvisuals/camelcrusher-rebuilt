#include "CamelCrusherVst3Editor.h"

#include "CamelCrusherVst3Controller.h"
#include "CamelCrusherVst3IDs.h"
#include "CamelCrusherVst3State.h"
#include "CamelCrusherMacEditorSurface.h"

#include "camelcrusher_recalled/ModernPluginModel.h"

#include "pluginterfaces/gui/iplugview.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

#import <AppKit/AppKit.h>

#include <array>
#include <cstring>
#include <random>

namespace camelcrusher_recalled::vst3 {

namespace {

const Steinberg::ViewRect kEditorBounds(0, 0, 345, 373);

constexpr CamelCrusherMacControlTagArray kVisualRoleTags{{
    0,   // Dist On
    2,   // Dist Tube
    1,   // Dist Mech
    3,   // Filter On
    4,   // Filter Cutoff
    5,   // Filter Res
    6,   // Compress On
    7,   // Compress Amount
    8,   // Compress Mode / Phat
    9,   // Master On
    11,  // Master Volume
    10,  // Master Mix
}};

CamelCrusherMacControlValueArray visualValuesForController(
    const CamelCrusherVst3Controller& controller) {
  const auto values = controller.publicParameterValues();
  return CamelCrusherMacControlValueArray{{
      values[0],  values[2],  values[1], values[3], values[4], values[5],
      values[6],  values[7],  values[8], values[9], values[11], values[10],
  }};
}

CamelCrusherMacEditorState makeEditorState(
    const CamelCrusherVst3Controller& controller) {
  CamelCrusherMacEditorState state;
  state.control_values = visualValuesForController(controller);
  state.program_names = controller.currentProgramNames();
  state.selected_program_index = controller.currentProgramSlotValue();
  state.show_randomize_button = true;
  return state;
}

}  // namespace

class CamelCrusherVst3EditorView final : public Steinberg::Vst::EditorView,
                                         public CamelCrusherVst3EditorObserver,
                                         public CamelCrusherMacEditorActionTarget {
 public:
  explicit CamelCrusherVst3EditorView(CamelCrusherVst3Controller& controller)
      : Steinberg::Vst::EditorView(&controller, const_cast<Steinberg::ViewRect*>(&kEditorBounds)),
        controller_(controller),
        surface_(this, kVisualRoleTags) {}

  ~CamelCrusherVst3EditorView() override {
    controller_.setEditorObserver(nullptr);
  }

  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(
      Steinberg::FIDString type) SMTG_OVERRIDE {
    if (type != nullptr && std::strcmp(type, Steinberg::kPlatformTypeNSView) == 0) {
      return Steinberg::kResultTrue;
    }
    return Steinberg::kResultFalse;
  }

  void attachedToParent() SMTG_OVERRIDE {
    auto* parent_view = (__bridge NSView*)systemWindow;
    if (parent_view == nullptr) {
      return;
    }

    controller_.setEditorObserver(this);
    NSView* editor_view = surface_.view();
    surface_.setFrame(NSMakeRect(0, 0, rect.getWidth(), rect.getHeight()));
    if (editor_view.superview != parent_view) {
      [parent_view addSubview:editor_view];
    }
    syncFromController();
  }

  void removedFromParent() SMTG_OVERRIDE {
    controller_.setEditorObserver(nullptr);
    [surface_.view() removeFromSuperview];
  }

  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* new_size) SMTG_OVERRIDE {
    const auto result = Steinberg::Vst::EditorView::onSize(new_size);
    if (result != Steinberg::kResultTrue || new_size == nullptr) {
      return result;
    }

    surface_.setFrame(NSMakeRect(0, 0, new_size->getWidth(), new_size->getHeight()));
    return result;
  }

  void vst3ControllerDidChangeState() override {
    syncFromController();
  }

  void previousProgramRequested() override {
    auto slot = controller_.currentProgramSlotValue();
    const auto slot_count =
        std::max<Steinberg::int32>(controller_.currentProgramNames().size(), 1);
    slot = (slot <= 0) ? slot_count - 1 : slot - 1;
    controller_.applyViewParameterEdit(
        kCamelCrusherProgramParameterId,
        normalizedProgramSlot(slot, slot_count));
  }

  void nextProgramRequested() override {
    auto slot = controller_.currentProgramSlotValue();
    const auto slot_count =
        std::max<Steinberg::int32>(controller_.currentProgramNames().size(), 1);
    slot = (slot + 1) % slot_count;
    controller_.applyViewParameterEdit(
        kCamelCrusherProgramParameterId,
        normalizedProgramSlot(slot, slot_count));
  }

  void programSelectionChanged(NSInteger index) override {
    const auto slot = static_cast<Steinberg::int32>(index);
    const auto slot_count =
        std::max<Steinberg::int32>(controller_.currentProgramNames().size(), 1);
    controller_.applyViewParameterEdit(
        kCamelCrusherProgramParameterId,
        normalizedProgramSlot(slot, slot_count));
  }

  void parameterControlChanged(NSInteger control_tag,
                               float normalized_value) override {
    controller_.applyViewParameterEdit(
        static_cast<Steinberg::Vst::ParamID>(control_tag),
        clampModernNormalizedValue(normalized_value));
  }

  void randomizeRequested() override {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> unit(0.0F, 1.0F);
    std::bernoulli_distribution toggle(0.5);

    controller_.applyViewParameterEdit(
        kCamelCrusherProgramParameterId,
        normalizedProgramSlot(kCamelCrusherCurrentStateProgramSlot,
                              std::max<Steinberg::int32>(
                                  controller_.currentProgramNames().size(), 1)));

    for (const auto& descriptor : modernPublicParameterDescriptors()) {
      float value = unit(rng);
      if (descriptor.public_index == 8 ||
          descriptor.value_kind == LegacyParameterValueKind::kSwitchLike) {
        value = toggle(rng) ? 1.0F : 0.0F;
      }
      controller_.applyViewParameterEdit(
          static_cast<Steinberg::Vst::ParamID>(descriptor.public_index), value);
    }
  }

 private:
  void syncFromController() {
    surface_.setState(makeEditorState(controller_));
  }

  CamelCrusherVst3Controller& controller_;
  CamelCrusherMacEditorSurface surface_;
};

Steinberg::IPlugView* createCamelCrusherVst3Editor(
    CamelCrusherVst3Controller& controller) {
  return new CamelCrusherVst3EditorView(controller);
}

}  // namespace camelcrusher_recalled::vst3
