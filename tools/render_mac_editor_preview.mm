#import <AppKit/AppKit.h>

#include "DefaultLegacyFactoryBank.h"
#include "CamelCrusherMacEditorSurface.h"

#include <string_view>

namespace {

constexpr CGFloat kPreviewScale = 2.0;
constexpr CGFloat kPreviewWidth = 345.0 * kPreviewScale;
constexpr CGFloat kPreviewHeight = 373.0 * kPreviewScale;

class PreviewActionTarget final
    : public camelcrusher_recalled::CamelCrusherMacEditorActionTarget {
 public:
  void previousProgramRequested() override {}
  void nextProgramRequested() override {}
  void programSelectionChanged(NSInteger /*index*/) override {}
  void parameterControlChanged(NSInteger /*control_tag*/,
                               float /*normalized_value*/) override {}
  void randomizeRequested() override {}
};

NSString* outputPathFromArgs(const int argc, char* argv[]) {
  for (int index = 1; index + 1 < argc; ++index) {
    if (std::string_view(argv[index]) == "--output") {
      return [NSString stringWithUTF8String:argv[index + 1]];
    }
  }
  return @"/tmp/camelcrusher-ui-preview-current.png";
}

camelcrusher_recalled::CamelCrusherMacEditorState makePreviewState() {
  using namespace camelcrusher_recalled;

  CamelCrusherMacEditorState state;
  const auto& bank = defaultLegacyFactoryBank();
  state.program_names.reserve(bank.presets.size());

  const LegacyPreset* selected_preset = nullptr;
  for (const auto& preset : bank.presets) {
    state.program_names.push_back(preset.name);
    if (preset.name == "British Clean") {
      selected_preset = &preset;
    }
  }

  if (selected_preset == nullptr && !bank.presets.empty()) {
    selected_preset = &bank.presets.front();
  }

  if (selected_preset != nullptr) {
    state.selected_program_index =
        static_cast<NSInteger>(selected_preset->program_index);
    state.control_values = {{
        selected_preset->state.dist_on,
        selected_preset->state.dist_tube,
        selected_preset->state.dist_mech,
        selected_preset->state.mm_filter_on,
        selected_preset->state.mm_filter_cutoff,
        selected_preset->state.mm_filter_res,
        selected_preset->state.compress_on,
        selected_preset->state.compress_amount,
        selected_preset->state.compress_mode,
        selected_preset->state.master_on,
        selected_preset->state.master_volume,
        selected_preset->state.master_mix,
    }};
  }

  state.show_randomize_button = true;
  return state;
}

}  // namespace

int main(int argc, char* argv[]) {
  @autoreleasepool {
    [NSApplication sharedApplication];

    PreviewActionTarget target;
    camelcrusher_recalled::CamelCrusherMacControlTagArray control_tags{{
        0, 2, 1, 3, 4, 5, 6, 7, 8, 9, 11, 10,
    }};

    camelcrusher_recalled::CamelCrusherMacEditorSurface surface(&target, control_tags);
    surface.setFrame(NSMakeRect(0.0, 0.0, kPreviewWidth, kPreviewHeight));
    surface.setState(makePreviewState());

    NSView* editor_view = surface.view();
    [editor_view layoutSubtreeIfNeeded];
    [editor_view displayIfNeeded];

    NSBitmapImageRep* bitmap =
        [editor_view bitmapImageRepForCachingDisplayInRect:editor_view.bounds];
    [bitmap setSize:editor_view.bounds.size];
    [editor_view cacheDisplayInRect:editor_view.bounds toBitmapImageRep:bitmap];

    NSData* png = [bitmap representationUsingType:NSBitmapImageFileTypePNG
                                       properties:@{}];
    return [png writeToFile:outputPathFromArgs(argc, argv) atomically:YES] ? 0 : 1;
  }
}
