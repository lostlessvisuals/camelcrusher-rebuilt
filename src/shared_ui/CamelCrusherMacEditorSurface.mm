#include "CamelCrusherMacEditorSurface.h"

#import <AppKit/AppKit.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifndef CAMELCRUSHER_RECALLED_SOURCE_DIR
#define CAMELCRUSHER_RECALLED_SOURCE_DIR ""
#endif

namespace camelcrusher_recalled {
class CamelCrusherMacEditorPanelOwner {
 public:
  virtual ~CamelCrusherMacEditorPanelOwner() = default;
  virtual void previousProgramRequested() = 0;
  virtual void nextProgramRequested() = 0;
  virtual void programSelectionChanged(NSPopUpButton* popup) = 0;
  virtual void parameterControlChanged(NSControl* control) = 0;
  virtual void randomizeRequested() = 0;
};
}  // namespace camelcrusher_recalled

@interface CamelCrusherMacKnobControl : NSControl
@end

@interface CamelCrusherMacSpriteToggleButton : NSButton
@property(nonatomic, strong) NSImage* spriteImage;
@property(nonatomic, assign) NSInteger frameCount;
@end

@interface CamelCrusherMacSpriteMomentaryButton : NSButton
@property(nonatomic, strong) NSImage* spriteImage;
@property(nonatomic, assign) NSInteger frameCount;
@property(nonatomic, assign) BOOL pressed;
@end

@interface CamelCrusherMacPatchArrowButton : NSButton
@property(nonatomic, assign) BOOL pointsLeft;
@end

@interface CamelCrusherMacEditorPanel : NSView
@property(nonatomic, assign) camelcrusher_recalled::CamelCrusherMacEditorPanelOwner* owner;
@property(nonatomic, copy) NSString* backgroundResourceName;
- (NSRect)brandLinkRect;
- (void)handlePreviousProgram:(id)sender;
- (void)handleNextProgram:(id)sender;
- (void)handleProgramChange:(id)sender;
- (void)handleParameterChange:(id)sender;
- (void)handleRandomize:(id)sender;
@end

namespace camelcrusher_recalled {
namespace {

constexpr CGFloat kOriginalWidth = 345.0;
constexpr CGFloat kOriginalHeight = 373.0;
constexpr CGFloat kBrandLinkX = 17.0;
constexpr CGFloat kBrandLinkY = 16.0;
constexpr CGFloat kBrandLinkWidth = 311.0;
constexpr CGFloat kBrandLinkHeight = 82.0;

struct ToggleLayout {
  CamelCrusherMacVisibleControlRole role;
  CGFloat x;
  CGFloat y;
  CGFloat width;
  CGFloat height;
  const char* resource_name;
};

struct KnobLayout {
  CamelCrusherMacVisibleControlRole role;
  CGFloat x;
  CGFloat y;
};

constexpr std::array<ToggleLayout, 5> kToggleLayouts{{
    {CamelCrusherMacVisibleControlRole::kDistOn, 39.0, 175.0, 32.0, 24.0,
     "OnButton.png"},
    {CamelCrusherMacVisibleControlRole::kFilterOn, 199.0, 175.0, 32.0, 24.0,
     "OnButton.png"},
    {CamelCrusherMacVisibleControlRole::kCompressOn, 39.0, 280.0, 32.0, 24.0,
     "OnButton.png"},
    {CamelCrusherMacVisibleControlRole::kMasterOn, 199.0, 280.0, 32.0, 24.0,
     "OnButton.png"},
    {CamelCrusherMacVisibleControlRole::kCompressMode, 123.0, 319.0, 22.0, 20.0,
     "PhatButton.png"},
}};

constexpr std::array<KnobLayout, 7> kKnobLayouts{{
    {CamelCrusherMacVisibleControlRole::kDistTube, 63.0, 214.0},
    {CamelCrusherMacVisibleControlRole::kDistMech, 123.0, 214.0},
    {CamelCrusherMacVisibleControlRole::kFilterCutoff, 222.0, 214.0},
    {CamelCrusherMacVisibleControlRole::kFilterRes, 282.0, 214.0},
    {CamelCrusherMacVisibleControlRole::kCompressAmount, 63.0, 319.0},
    {CamelCrusherMacVisibleControlRole::kMasterVolume, 222.0, 319.0},
    {CamelCrusherMacVisibleControlRole::kMasterMix, 282.0, 319.0},
}};

constexpr CGFloat kPresetSelectorX = 23.0;
constexpr CGFloat kPresetSelectorY = 113.0;
constexpr CGFloat kPresetSelectorWidth = 196.0;
constexpr CGFloat kPresetSelectorHeight = 30.0;

constexpr CGFloat kPresetPopupX = 53.0;
constexpr CGFloat kPresetPopupY = 117.0;
constexpr CGFloat kPresetPopupWidth = 118.0;
constexpr CGFloat kPresetPopupHeight = 18.0;
constexpr CGFloat kPresetPopupFontSize = 14.0;
constexpr CGFloat kPresetMenuFontSize = 15.0;

constexpr CGFloat kPresetPreviousWidth = 29.0;
constexpr CGFloat kPresetNextWidth = 30.0;

constexpr CGFloat kRandomizeCenterX = 275.0;
constexpr CGFloat kRandomizeCenterY = 127.0;
constexpr CGFloat kRandomizeWidth = 93.0;
constexpr CGFloat kRandomizeHeight = 28.0;

std::size_t roleIndex(const CamelCrusherMacVisibleControlRole role) {
  return static_cast<std::size_t>(role);
}

NSFont* CamelCrusherProgramFont(CGFloat size) {
  NSFont* font = [NSFont fontWithName:@"Menlo Bold" size:size];
  return font != nil ? font : [NSFont monospacedSystemFontOfSize:size
                                                          weight:NSFontWeightBold];
}

NSColor* CamelCrusherProgramTextColor() {
  return [NSColor colorWithCalibratedRed:0.90 green:0.93 blue:1.0 alpha:1.0];
}

NSString* camelCrusherBrandURLString() {
  NSString* override =
      [NSProcessInfo processInfo].environment[@"CAMELCRUSHER_BRAND_URL"];
  if (override.length > 0U) {
    return override;
  }
#ifdef CAMELCRUSHER_BRAND_URL
  return @CAMELCRUSHER_BRAND_URL;
#else
  return @"https://lostless.live";
#endif
}

void openCamelCrusherBrandURL() {
  NSString* url_string = camelCrusherBrandURLString();
  if (url_string.length == 0U) {
    return;
  }

  NSURL* url = [NSURL URLWithString:url_string];
  if (url == nil) {
    return;
  }

  [[NSWorkspace sharedWorkspace] openURL:url];
}

NSString* camelCrusherSourceResourcePath(NSString* resource_name) {
  if (std::strlen(CAMELCRUSHER_RECALLED_SOURCE_DIR) == 0) {
    return nil;
  }

  NSString* source_root =
      [NSString stringWithUTF8String:CAMELCRUSHER_RECALLED_SOURCE_DIR];
  return [source_root
      stringByAppendingPathComponent:[NSString
                                         stringWithFormat:@"src/compat_vst2/resources/%@",
                                                          resource_name]];
}

NSImage* camelCrusherResourceImage(NSString* resource_name) {
  static NSMutableDictionary<NSString*, NSImage*>* cache = nil;
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    cache = [[NSMutableDictionary alloc] init];
  });

  NSString* cache_key = [resource_name copy];
  NSImage* cached = cache[cache_key];
  if (cached != nil) {
    return cached;
  }

  NSBundle* bundle = [NSBundle bundleForClass:[CamelCrusherMacEditorPanel class]];
  NSString* path = [bundle pathForResource:[resource_name stringByDeletingPathExtension]
                                    ofType:[resource_name pathExtension]];
  if (path == nil) {
    path = camelCrusherSourceResourcePath(resource_name);
  }

  NSImage* image =
      path != nil ? [[NSImage alloc] initWithContentsOfFile:path] : nil;
  if (image != nil) {
    cache[cache_key] = image;
  }
  return image;
}

void drawSpriteFrame(NSImage* image,
                     NSInteger frame_index,
                     NSInteger frame_count,
                     const NSRect destination_rect) {
  if (image == nil || frame_count <= 0) {
    return;
  }

  CGImageRef cg_image =
      [image CGImageForProposedRect:nullptr context:nil hints:nil];
  if (cg_image == nullptr) {
    return;
  }

  const CGFloat width = static_cast<CGFloat>(CGImageGetWidth(cg_image));
  const CGFloat height = static_cast<CGFloat>(CGImageGetHeight(cg_image));
  const CGFloat frame_height = height / static_cast<CGFloat>(frame_count);
  const CGFloat clamped_index =
      std::clamp(static_cast<CGFloat>(frame_index), 0.0,
                 static_cast<CGFloat>(frame_count - 1));
  const CGRect source_rect =
      CGRectMake(0.0,
                 height - ((clamped_index + 1.0) * frame_height),
                 width,
                 frame_height);
  CGImageRef frame_image = CGImageCreateWithImageInRect(cg_image, source_rect);
  if (frame_image == nullptr) {
    return;
  }

  NSImage* image_frame =
      [[NSImage alloc] initWithCGImage:frame_image
                                  size:NSMakeSize(CGImageGetWidth(frame_image),
                                                  CGImageGetHeight(frame_image))];
  [image_frame drawInRect:destination_rect
                 fromRect:NSZeroRect
                operation:NSCompositingOperationSourceOver
                 fraction:1.0
           respectFlipped:YES
                    hints:nil];
  CGImageRelease(frame_image);
}

NSAttributedString* styledProgramTitle(NSString* title) {
  NSMutableParagraphStyle* paragraph = [[NSMutableParagraphStyle alloc] init];
  paragraph.lineBreakMode = NSLineBreakByTruncatingTail;

  NSShadow* shadow = [[NSShadow alloc] init];
  shadow.shadowOffset = NSMakeSize(0.0, -1.0);
  shadow.shadowBlurRadius = 2.0;
  shadow.shadowColor = [NSColor colorWithCalibratedWhite:0.0 alpha:0.55];

  return [[NSAttributedString alloc] initWithString:title
                                         attributes:@{
                                           NSFontAttributeName :
                                               CamelCrusherProgramFont(
                                                   kPresetMenuFontSize),
                                           NSForegroundColorAttributeName :
                                               CamelCrusherProgramTextColor(),
                                           NSParagraphStyleAttributeName : paragraph,
                                           NSShadowAttributeName : shadow,
                                         }];
}

NSRect scaledRect(const CGFloat scale_x,
                  const CGFloat scale_y,
                  const CGFloat offset_x,
                  const CGFloat offset_y,
                  const CGFloat x,
                  const CGFloat y,
                  const CGFloat width,
                  const CGFloat height) {
  return NSMakeRect(std::round(offset_x + (x * scale_x)),
                    std::round(offset_y + (y * scale_y)),
                    std::round(width * scale_x), std::round(height * scale_y));
}

NSRect scaledCenteredRect(const CGFloat scale_x,
                          const CGFloat scale_y,
                          const CGFloat offset_x,
                          const CGFloat offset_y,
                          const CGFloat center_x,
                          const CGFloat center_y,
                          const CGFloat width,
                          const CGFloat height) {
  return scaledRect(scale_x, scale_y, offset_x, offset_y, center_x - (width * 0.5),
                    center_y - (height * 0.5), width, height);
}

}  // namespace
}  // namespace camelcrusher_recalled

@implementation CamelCrusherMacKnobControl {
  double dragOriginValue_;
  CGFloat dragOriginY_;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  return self;
}

- (BOOL)isOpaque {
  return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  NSImage* knob_sprite = camelcrusher_recalled::camelCrusherResourceImage(@"Knob.png");
  if (knob_sprite != nil) {
    const NSInteger frame_count = 55;
    const CGFloat value =
        std::clamp(static_cast<CGFloat>(self.doubleValue), 0.0, 1.0);
    const NSInteger frame_index = static_cast<NSInteger>(
        std::llround(value * static_cast<CGFloat>(frame_count - 1)));
    camelcrusher_recalled::drawSpriteFrame(knob_sprite, frame_index, frame_count,
                                           self.bounds);
  }
}

- (void)mouseDown:(NSEvent*)event {
  dragOriginValue_ = self.doubleValue;
  dragOriginY_ = [self convertPoint:event.locationInWindow fromView:nil].y;

  BOOL keep_tracking = YES;
  while (keep_tracking) {
    NSEvent* next_event = [self.window
        nextEventMatchingMask:NSEventMaskLeftMouseUp | NSEventMaskLeftMouseDragged];
    if (next_event.type == NSEventTypeLeftMouseUp) {
      keep_tracking = NO;
      continue;
    }

    const CGFloat current_y =
        [self convertPoint:next_event.locationInWindow fromView:nil].y;
    const CGFloat delta = (dragOriginY_ - current_y) / 160.0;
    const double new_value =
        std::clamp(dragOriginValue_ + static_cast<double>(delta), 0.0, 1.0);
    if (std::fabs(new_value - self.doubleValue) > 0.00001) {
      self.doubleValue = new_value;
      [self setNeedsDisplay:YES];
      [self sendAction:self.action to:self.target];
    }
  }
}

@end

@implementation CamelCrusherMacSpriteToggleButton

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self != nil) {
    [self setButtonType:NSButtonTypePushOnPushOff];
    self.bordered = NO;
    self.title = @"";
    self.frameCount = 2;
  }
  return self;
}

- (BOOL)isOpaque {
  return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];
  const NSInteger frame_index =
      self.state == NSControlStateValueOn ? 0 : self.frameCount - 1;
  camelcrusher_recalled::drawSpriteFrame(self.spriteImage, frame_index,
                                         self.frameCount, self.bounds);
}

- (void)mouseDown:(NSEvent*)event {
  self.state = (self.state == NSControlStateValueOn) ? NSControlStateValueOff
                                                     : NSControlStateValueOn;
  [self setNeedsDisplay:YES];
  [self sendAction:self.action to:self.target];
}

@end

@implementation CamelCrusherMacSpriteMomentaryButton

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self != nil) {
    self.bordered = NO;
    self.title = @"";
    self.frameCount = 2;
  }
  return self;
}

- (BOOL)isOpaque {
  return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];
  const NSInteger frame_index = self.pressed ? 1 : 0;
  camelcrusher_recalled::drawSpriteFrame(self.spriteImage, frame_index,
                                         self.frameCount, self.bounds);
}

- (void)mouseDown:(NSEvent*)event {
  self.pressed = YES;
  [self setNeedsDisplay:YES];
  [self sendAction:self.action to:self.target];
  self.pressed = NO;
  [self setNeedsDisplay:YES];
}

@end

@implementation CamelCrusherMacPatchArrowButton

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self != nil) {
    self.bordered = NO;
    self.title = @"";
  }
  return self;
}

- (BOOL)isOpaque {
  return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];
}

- (void)mouseDown:(NSEvent*)event {
  [self sendAction:self.action to:self.target];
}

@end

@implementation CamelCrusherMacEditorPanel

- (BOOL)isFlipped {
  return YES;
}

- (BOOL)isOpaque {
  return YES;
}

- (NSRect)brandLinkRect {
  const CGFloat scale =
      std::min(self.bounds.size.width / camelcrusher_recalled::kOriginalWidth,
               self.bounds.size.height / camelcrusher_recalled::kOriginalHeight);
  const CGFloat content_width =
      std::round(camelcrusher_recalled::kOriginalWidth * scale);
  const CGFloat content_height =
      std::round(camelcrusher_recalled::kOriginalHeight * scale);
  const CGFloat offset_x =
      std::round((self.bounds.size.width - content_width) * 0.5);
  const CGFloat offset_y =
      std::round((self.bounds.size.height - content_height) * 0.5);
  return camelcrusher_recalled::scaledRect(
      scale, scale, offset_x, offset_y, camelcrusher_recalled::kBrandLinkX,
      camelcrusher_recalled::kBrandLinkY,
      camelcrusher_recalled::kBrandLinkWidth,
      camelcrusher_recalled::kBrandLinkHeight);
}

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  const CGFloat scale =
      std::min(self.bounds.size.width / camelcrusher_recalled::kOriginalWidth,
               self.bounds.size.height / camelcrusher_recalled::kOriginalHeight);
  const CGFloat content_width =
      std::round(camelcrusher_recalled::kOriginalWidth * scale);
  const CGFloat content_height =
      std::round(camelcrusher_recalled::kOriginalHeight * scale);
  const NSRect content_rect =
      NSMakeRect(std::round((self.bounds.size.width - content_width) * 0.5),
                 std::round((self.bounds.size.height - content_height) * 0.5),
                 content_width, content_height);

  NSImage* background =
      camelcrusher_recalled::camelCrusherResourceImage(
          self.backgroundResourceName ?: @"Background.png");
  if (background != nil) {
    [background drawInRect:content_rect
                  fromRect:NSZeroRect
                 operation:NSCompositingOperationSourceOver
                  fraction:1.0
            respectFlipped:YES
                     hints:nil];
    return;
  }

  [[NSColor colorWithCalibratedWhite:0.08 alpha:1.0] setFill];
  NSRectFill(self.bounds);
}

- (void)mouseDown:(NSEvent*)event {
  const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
  if (NSPointInRect(point, self.brandLinkRect)) {
    camelcrusher_recalled::openCamelCrusherBrandURL();
    return;
  }
  [super mouseDown:event];
}

- (void)resetCursorRects {
  [super resetCursorRects];
  [self addCursorRect:self.brandLinkRect cursor:[NSCursor pointingHandCursor]];
}

- (void)handlePreviousProgram:(id)sender {
  if (self.owner != nullptr) {
    self.owner->previousProgramRequested();
  }
}

- (void)handleNextProgram:(id)sender {
  if (self.owner != nullptr) {
    self.owner->nextProgramRequested();
  }
}

- (void)handleProgramChange:(id)sender {
  if (self.owner != nullptr) {
    self.owner->programSelectionChanged((NSPopUpButton*)sender);
  }
}

- (void)handleParameterChange:(id)sender {
  if (self.owner != nullptr) {
    self.owner->parameterControlChanged((NSControl*)sender);
  }
}

- (void)handleRandomize:(id)sender {
  if (self.owner != nullptr) {
    self.owner->randomizeRequested();
  }
}

@end

namespace camelcrusher_recalled {

struct CamelCrusherMacEditorSurface::Impl : CamelCrusherMacEditorPanelOwner {
  Impl(CamelCrusherMacEditorActionTarget* owner,
       const CamelCrusherMacControlTagArray& control_tags,
       NSString* background_resource_name)
      : owner(owner),
        control_tags(control_tags),
        background_resource_name([background_resource_name copy]) {
    buildViewHierarchy();
  }

  void previousProgramRequested() override {
    if (owner != nullptr) {
      owner->previousProgramRequested();
    }
  }

  void nextProgramRequested() override {
    if (owner != nullptr) {
      owner->nextProgramRequested();
    }
  }

  void programSelectionChanged(NSPopUpButton* popup) override {
    if (owner != nullptr && popup != nil) {
      owner->programSelectionChanged(popup.indexOfSelectedItem);
    }
  }

  void parameterControlChanged(NSControl* control) override {
    if (owner == nullptr || control == nil) {
      return;
    }

    float value = 0.0F;
    if ([control isKindOfClass:[CamelCrusherMacSpriteToggleButton class]]) {
      auto* toggle = static_cast<CamelCrusherMacSpriteToggleButton*>(control);
      value = toggle.state == NSControlStateValueOn ? 1.0F : 0.0F;
    } else {
      auto* knob = static_cast<CamelCrusherMacKnobControl*>(control);
      value = static_cast<float>(knob.doubleValue);
    }
    owner->parameterControlChanged(control.tag, value);
  }

  void randomizeRequested() override {
    if (owner != nullptr) {
      owner->randomizeRequested();
    }
  }

  void setState(const CamelCrusherMacEditorState& state) {
    refreshProgramMenu(state.program_names, state.selected_program_index);

    for (std::size_t index = 0; index < control_views.size(); ++index) {
      NSControl* control = control_views[index];
      if (control == nil) {
        continue;
      }

      const float value = std::clamp(state.control_values[index], 0.0F, 1.0F);
      if ([control isKindOfClass:[CamelCrusherMacSpriteToggleButton class]]) {
        auto* toggle = static_cast<CamelCrusherMacSpriteToggleButton*>(control);
        toggle.state =
            value >= 0.5F ? NSControlStateValueOn : NSControlStateValueOff;
      } else {
        auto* knob = static_cast<CamelCrusherMacKnobControl*>(control);
        knob.doubleValue = value;
      }
      [control setNeedsDisplay:YES];
    }

    randomize_button_.hidden = !state.show_randomize_button;
    [randomize_button_ setNeedsDisplay:YES];
  }

  void layoutViewHierarchy() {
    const CGFloat root_width = root_view_.bounds.size.width;
    const CGFloat root_height = root_view_.bounds.size.height;
    const CGFloat scale =
        std::min(root_width / kOriginalWidth, root_height / kOriginalHeight);
    const CGFloat content_width = std::round(kOriginalWidth * scale);
    const CGFloat content_height = std::round(kOriginalHeight * scale);
    const CGFloat offset_x = std::round((root_width - content_width) * 0.5);
    const CGFloat offset_y = std::round((root_height - content_height) * 0.5);
    const CGFloat scale_x = scale;
    const CGFloat scale_y = scale;

    preset_selector_background_.frame = scaledRect(
        scale_x, scale_y, offset_x, offset_y, kPresetSelectorX, kPresetSelectorY,
        kPresetSelectorWidth, kPresetSelectorHeight);
    preset_popup_.frame =
        scaledRect(scale_x, scale_y, offset_x, offset_y, kPresetPopupX,
                   kPresetPopupY, kPresetPopupWidth, kPresetPopupHeight);
    previous_program_button_.frame =
        scaledRect(scale_x, scale_y, offset_x, offset_y, kPresetSelectorX,
                   kPresetSelectorY, kPresetPreviousWidth, kPresetSelectorHeight);
    next_program_button_.frame =
        scaledRect(scale_x, scale_y, offset_x, offset_y, 189.0, kPresetSelectorY,
                   kPresetNextWidth, kPresetSelectorHeight);

    randomize_button_.frame =
        scaledCenteredRect(scale_x, scale_y, offset_x, offset_y,
                           kRandomizeCenterX, kRandomizeCenterY, kRandomizeWidth,
                           kRandomizeHeight);

    for (const auto& layout : kToggleLayouts) {
      auto* control = static_cast<CamelCrusherMacSpriteToggleButton*>(
          control_views[roleIndex(layout.role)]);
      control.frame = scaledCenteredRect(scale_x, scale_y, offset_x, offset_y,
                                         layout.x, layout.y, layout.width,
                                         layout.height);
    }

    for (const auto& layout : kKnobLayouts) {
      auto* control = static_cast<CamelCrusherMacKnobControl*>(
          control_views[roleIndex(layout.role)]);
      control.frame = scaledCenteredRect(scale_x, scale_y, offset_x, offset_y,
                                         layout.x, layout.y, 48.0, 48.0);
    }
  }

  void refreshProgramMenu(const std::vector<std::string>& program_names,
                          const NSInteger selected_program_index) {
    [preset_popup_ removeAllItems];
    if (program_names.empty()) {
      [preset_popup_ addItemWithTitle:@"Current State"];
      preset_popup_.lastItem.attributedTitle =
          styledProgramTitle(@"Current State");
      [preset_popup_ selectItemAtIndex:0];
      return;
    }

    for (const auto& name : program_names) {
      NSString* title = [NSString stringWithUTF8String:name.c_str()];
      [preset_popup_ addItemWithTitle:title];
      preset_popup_.lastItem.attributedTitle = styledProgramTitle(title);
    }

    const NSInteger clamped_index =
        std::clamp(selected_program_index, static_cast<NSInteger>(0),
                   static_cast<NSInteger>(program_names.size() - 1));
    [preset_popup_ selectItemAtIndex:clamped_index];
  }

  void buildViewHierarchy() {
    root_view_ = [[CamelCrusherMacEditorPanel alloc]
        initWithFrame:NSMakeRect(0.0, 0.0, kOriginalWidth, kOriginalHeight)];
    root_view_.owner = this;
    root_view_.backgroundResourceName = background_resource_name;

    preset_selector_background_ =
        [[NSImageView alloc] initWithFrame:NSZeroRect];
    preset_selector_background_.image =
        camelCrusherResourceImage(@"SelectorPreset-Default.png");
    preset_selector_background_.imageScaling = NSImageScaleAxesIndependently;
    [root_view_ addSubview:preset_selector_background_];

    preset_popup_ = [[NSPopUpButton alloc] initWithFrame:NSZeroRect pullsDown:NO];
    preset_popup_.target = root_view_;
    preset_popup_.action = @selector(handleProgramChange:);
    preset_popup_.bordered = NO;
    preset_popup_.focusRingType = NSFocusRingTypeNone;
    preset_popup_.font = CamelCrusherProgramFont(kPresetPopupFontSize);
    auto* popup_cell = static_cast<NSPopUpButtonCell*>(preset_popup_.cell);
    popup_cell.arrowPosition = NSPopUpNoArrow;
    popup_cell.bordered = NO;
    [root_view_ addSubview:preset_popup_];

    previous_program_button_ =
        [[CamelCrusherMacPatchArrowButton alloc] initWithFrame:NSZeroRect];
    previous_program_button_.pointsLeft = YES;
    previous_program_button_.target = root_view_;
    previous_program_button_.action = @selector(handlePreviousProgram:);
    [root_view_ addSubview:previous_program_button_];

    next_program_button_ =
        [[CamelCrusherMacPatchArrowButton alloc] initWithFrame:NSZeroRect];
    next_program_button_.pointsLeft = NO;
    next_program_button_.target = root_view_;
    next_program_button_.action = @selector(handleNextProgram:);
    [root_view_ addSubview:next_program_button_];

    randomize_button_ =
        [[CamelCrusherMacSpriteMomentaryButton alloc] initWithFrame:NSZeroRect];
    randomize_button_.spriteImage = camelCrusherResourceImage(@"ButtonRandom.png");
    randomize_button_.target = root_view_;
    randomize_button_.action = @selector(handleRandomize:);
    [root_view_ addSubview:randomize_button_];

    for (const auto& layout : kToggleLayouts) {
      auto* toggle =
          [[CamelCrusherMacSpriteToggleButton alloc] initWithFrame:NSZeroRect];
      toggle.tag = control_tags[roleIndex(layout.role)];
      toggle.spriteImage =
          camelCrusherResourceImage([NSString stringWithUTF8String:layout.resource_name]);
      toggle.target = root_view_;
      toggle.action = @selector(handleParameterChange:);
      control_views[roleIndex(layout.role)] = toggle;
      [root_view_ addSubview:toggle];
    }

    for (const auto& layout : kKnobLayouts) {
      auto* knob = [[CamelCrusherMacKnobControl alloc] initWithFrame:NSZeroRect];
      knob.tag = control_tags[roleIndex(layout.role)];
      knob.target = root_view_;
      knob.action = @selector(handleParameterChange:);
      control_views[roleIndex(layout.role)] = knob;
      [root_view_ addSubview:knob];
    }

    layoutViewHierarchy();
  }

  CamelCrusherMacEditorActionTarget* owner = nullptr;
  CamelCrusherMacControlTagArray control_tags{};
  NSString* background_resource_name = nil;

  CamelCrusherMacEditorPanel* root_view_ = nil;
  NSImageView* preset_selector_background_ = nil;
  NSPopUpButton* preset_popup_ = nil;
  CamelCrusherMacPatchArrowButton* previous_program_button_ = nil;
  CamelCrusherMacPatchArrowButton* next_program_button_ = nil;
  CamelCrusherMacSpriteMomentaryButton* randomize_button_ = nil;
  std::array<NSControl*, kCamelCrusherMacVisibleControlCount> control_views{};
};

CamelCrusherMacEditorSurface::CamelCrusherMacEditorSurface(
    CamelCrusherMacEditorActionTarget* owner,
    const CamelCrusherMacControlTagArray& control_tags,
    NSString* background_resource_name)
    : impl_(std::make_unique<Impl>(owner, control_tags,
                                   background_resource_name)) {}

CamelCrusherMacEditorSurface::~CamelCrusherMacEditorSurface() = default;

NSView* CamelCrusherMacEditorSurface::view() const {
  return impl_->root_view_;
}

void CamelCrusherMacEditorSurface::setFrame(const NSRect frame) {
  impl_->root_view_.frame = frame;
  impl_->layoutViewHierarchy();
  [impl_->root_view_ setNeedsDisplay:YES];
}

void CamelCrusherMacEditorSurface::setState(const CamelCrusherMacEditorState& state) {
  impl_->setState(state);
}

}  // namespace camelcrusher_recalled
