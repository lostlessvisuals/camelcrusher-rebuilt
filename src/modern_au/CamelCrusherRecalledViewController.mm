#import "CamelCrusherRecalledViewController.h"

#import "camelcrusher_recalled/CamelCrusherRecalledAudioUnit.h"

#include "CamelCrusherMacEditorSurface.h"

#import <AudioToolbox/AUAudioUnitImplementation.h>
#import <AudioToolbox/AudioToolbox.h>

#include <array>
#include <cmath>
#include <random>
#include <string>

namespace camelcrusher_recalled {
namespace {

constexpr CGFloat kEditorWidth = 345.0;
constexpr CGFloat kEditorHeight = 373.0;

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

bool visualRoleUsesToggle(const std::size_t index) {
  switch (static_cast<CamelCrusherMacVisibleControlRole>(index)) {
    case CamelCrusherMacVisibleControlRole::kDistOn:
    case CamelCrusherMacVisibleControlRole::kFilterOn:
    case CamelCrusherMacVisibleControlRole::kCompressOn:
    case CamelCrusherMacVisibleControlRole::kCompressMode:
    case CamelCrusherMacVisibleControlRole::kMasterOn:
      return true;
    default:
      return false;
  }
}

CamelCrusherMacEditorState makeEditorState(
    CamelCrusherRecalledAudioUnit* audio_unit) {
  CamelCrusherMacEditorState state;
  for (std::size_t index = 0; index < kVisualRoleTags.size(); ++index) {
    AUParameter* parameter =
        [audio_unit.parameterTree parameterWithAddress:kVisualRoleTags[index]];
    state.control_values[index] = parameter != nil ? parameter.value : 0.0F;
  }

  NSString* current_title =
      audio_unit.currentPreset != nil ? audio_unit.currentPreset.name : @"Current State";
  state.program_names.emplace_back(current_title.UTF8String ?: "Current State");
  for (AUAudioUnitPreset* preset in audio_unit.factoryPresets) {
    state.program_names.emplace_back(preset.name.UTF8String ?: "Factory Preset");
  }

  if (audio_unit.currentPreset != nil && audio_unit.currentPreset.number >= 0) {
    state.selected_program_index = audio_unit.currentPreset.number + 1;
  } else {
    state.selected_program_index = 0;
  }
  state.show_randomize_button = true;
  return state;
}

class CamelCrusherRecalledUIViewAdapter final
    : public CamelCrusherMacEditorActionTarget {
 public:
  explicit CamelCrusherRecalledUIViewAdapter(
      CamelCrusherRecalledViewController* controller)
      : controller_(controller) {}

  void previousProgramRequested() override;
  void nextProgramRequested() override;
  void programSelectionChanged(NSInteger index) override;
  void parameterControlChanged(NSInteger control_tag,
                               float normalized_value) override;
  void randomizeRequested() override;

 private:
  __weak CamelCrusherRecalledViewController* controller_ = nil;
};

}  // namespace
}  // namespace camelcrusher_recalled

@interface CamelCrusherRecalledViewController () {
  CamelCrusherRecalledAudioUnit* _audioUnit;
  AUParameterObserverToken _parameterObserverToken;
  std::unique_ptr<camelcrusher_recalled::CamelCrusherRecalledUIViewAdapter>
      _adapter;
  std::unique_ptr<camelcrusher_recalled::CamelCrusherMacEditorSurface> _surface;
}

- (void)commonInit;
- (void)attachAudioUnit:(CamelCrusherRecalledAudioUnit*)audioUnit;
- (void)applyProgramSelectionIndex:(NSInteger)index;
- (void)applyParameterControlTag:(NSInteger)controlTag value:(float)value;
- (void)detachParameterObserverIfNeeded;
- (void)layoutEditorSurface;
- (void)performRandomize;
- (void)registerParameterObserverIfNeeded;
- (void)syncFromAudioUnit;
- (void)selectRelativeProgramOffset:(NSInteger)delta;

@end

@implementation CamelCrusherRecalledViewController

- (instancetype)init {
  return [self initWithNibName:nil bundle:nil];
}

- (instancetype)initWithCoder:(NSCoder*)coder {
  self = [super initWithCoder:coder];
  if (self != nil) {
    [self commonInit];
  }
  return self;
}

- (instancetype)initWithNibName:(NSNibName)nibNameOrNil
                         bundle:(NSBundle*)nibBundleOrNil {
  self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
  if (self != nil) {
    [self commonInit];
  }
  return self;
}

- (instancetype)initWithAudioUnit:(CamelCrusherRecalledAudioUnit*)audioUnit {
  self = [self init];
  if (self != nil) {
    [self attachAudioUnit:audioUnit];
  }
  return self;
}

- (void)dealloc {
  [self detachParameterObserverIfNeeded];
}

- (void)loadView {
  NSView* container =
      [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0,
                                               camelcrusher_recalled::kEditorWidth,
                                               camelcrusher_recalled::kEditorHeight)];
  [container addSubview:_surface->view()];
  self.view = container;
  self.preferredContentSize =
      NSMakeSize(camelcrusher_recalled::kEditorWidth,
                 camelcrusher_recalled::kEditorHeight);
  [self layoutEditorSurface];
  [self syncFromAudioUnit];
}

- (void)viewDidLayout {
  [super viewDidLayout];
  [self layoutEditorSurface];
}

- (AUAudioUnit*)createAudioUnitWithComponentDescription:(AudioComponentDescription)desc
                                                  error:(NSError* _Nullable __autoreleasing*)error {
  if (_audioUnit != nil) {
    const AudioComponentDescription current =
        [CamelCrusherRecalledAudioUnit componentDescription];
    if (current.componentType == desc.componentType &&
        current.componentSubType == desc.componentSubType &&
        current.componentManufacturer == desc.componentManufacturer) {
      return _audioUnit;
    }
  }

  CamelCrusherRecalledAudioUnit* audio_unit =
      [[CamelCrusherRecalledAudioUnit alloc] initWithComponentDescription:desc
                                                                  options:0
                                                                    error:error];
  if (audio_unit != nil) {
    [self attachAudioUnit:audio_unit];
  }
  return audio_unit;
}

- (void)commonInit {
  if (_adapter != nullptr || _surface != nullptr) {
    return;
  }

  _adapter = std::make_unique<
      camelcrusher_recalled::CamelCrusherRecalledUIViewAdapter>(self);
  _surface = std::make_unique<camelcrusher_recalled::CamelCrusherMacEditorSurface>(
      _adapter.get(), camelcrusher_recalled::kVisualRoleTags);
}

- (void)attachAudioUnit:(CamelCrusherRecalledAudioUnit*)audioUnit {
  if (_audioUnit == audioUnit) {
    return;
  }

  [self detachParameterObserverIfNeeded];
  _audioUnit = audioUnit;
  [self registerParameterObserverIfNeeded];

  if (self.isViewLoaded) {
    [self syncFromAudioUnit];
  }
}

- (void)detachParameterObserverIfNeeded {
  if (_audioUnit.parameterTree != nil && _parameterObserverToken != nil) {
    [_audioUnit.parameterTree removeParameterObserver:_parameterObserverToken];
  }
  _parameterObserverToken = nil;
}

- (void)layoutEditorSurface {
  if (_surface == nullptr || self.view == nil) {
    return;
  }

  const NSRect bounds = self.view.bounds;
  const CGFloat x =
      std::floor((NSWidth(bounds) - camelcrusher_recalled::kEditorWidth) * 0.5);
  const CGFloat y =
      std::floor((NSHeight(bounds) - camelcrusher_recalled::kEditorHeight) * 0.5);
  _surface->setFrame(
      NSMakeRect(x, y, camelcrusher_recalled::kEditorWidth,
                 camelcrusher_recalled::kEditorHeight));
}

- (void)registerParameterObserverIfNeeded {
  if (_audioUnit == nil || _audioUnit.parameterTree == nil ||
      _parameterObserverToken != nil) {
    return;
  }

  __weak __typeof__(self) weak_self = self;
  _parameterObserverToken =
      [_audioUnit.parameterTree tokenByAddingParameterObserver:^(
                                 AUParameterAddress address, AUValue value) {
        (void)address;
        (void)value;
        dispatch_async(dispatch_get_main_queue(), ^{
          [weak_self syncFromAudioUnit];
        });
      }];
}

- (void)syncFromAudioUnit {
  if (_audioUnit == nil || _surface == nullptr) {
    return;
  }
  _surface->setState(camelcrusher_recalled::makeEditorState(_audioUnit));
}

- (void)selectRelativeProgramOffset:(NSInteger)delta {
  const NSInteger slot_count =
      static_cast<NSInteger>(_audioUnit.factoryPresets.count) + 1;
  NSInteger current_slot = 0;
  if (_audioUnit.currentPreset != nil && _audioUnit.currentPreset.number >= 0) {
    current_slot = _audioUnit.currentPreset.number + 1;
  }
  NSInteger next_slot = (current_slot + delta) % slot_count;
  if (next_slot < 0) {
    next_slot += slot_count;
  }
  [self applyProgramSelectionIndex:next_slot];
}

- (void)applyProgramSelectionIndex:(NSInteger)index {
  if (_audioUnit == nil) {
    return;
  }

  if (index <= 0) {
    _audioUnit.currentPreset = nil;
    [self syncFromAudioUnit];
    return;
  }

  const NSInteger factory_index = index - 1;
  if (factory_index >= 0 &&
      factory_index < static_cast<NSInteger>(_audioUnit.factoryPresets.count)) {
    _audioUnit.currentPreset = _audioUnit.factoryPresets[factory_index];
    [self syncFromAudioUnit];
  }
}

- (void)applyParameterControlTag:(NSInteger)controlTag value:(float)value {
  if (_audioUnit == nil) {
    return;
  }

  AUParameter* parameter =
      [_audioUnit.parameterTree parameterWithAddress:static_cast<AUParameterAddress>(
                                               controlTag)];
  if (parameter == nil) {
    return;
  }

  [parameter setValue:value originator:_parameterObserverToken];
}

- (void)performRandomize {
  if (_audioUnit == nil) {
    return;
  }

  _audioUnit.currentPreset = nil;

  std::mt19937 rng(std::random_device{}());
  std::uniform_real_distribution<float> unit(0.0F, 1.0F);
  std::bernoulli_distribution toggle(0.5);

  for (std::size_t index = 0; index < camelcrusher_recalled::kVisualRoleTags.size();
       ++index) {
    AUParameter* parameter =
        [_audioUnit.parameterTree parameterWithAddress:camelcrusher_recalled::kVisualRoleTags[index]];
    if (parameter == nil) {
      continue;
    }

    float randomized = unit(rng);
    if (camelcrusher_recalled::visualRoleUsesToggle(index)) {
      randomized = toggle(rng) ? 1.0F : 0.0F;
    }
    [parameter setValue:randomized originator:_parameterObserverToken];
  }
}

@end

namespace camelcrusher_recalled {
namespace {

void CamelCrusherRecalledUIViewAdapter::previousProgramRequested() {
  [controller_ selectRelativeProgramOffset:-1];
}

void CamelCrusherRecalledUIViewAdapter::nextProgramRequested() {
  [controller_ selectRelativeProgramOffset:1];
}

void CamelCrusherRecalledUIViewAdapter::programSelectionChanged(NSInteger index) {
  [controller_ applyProgramSelectionIndex:index];
}

void CamelCrusherRecalledUIViewAdapter::parameterControlChanged(
    NSInteger control_tag,
    float normalized_value) {
  [controller_ applyParameterControlTag:control_tag value:normalized_value];
}

void CamelCrusherRecalledUIViewAdapter::randomizeRequested() {
  [controller_ performRandomize];
}

}  // namespace
}  // namespace camelcrusher_recalled
