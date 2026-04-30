#include "CamelCrusherVst2Editor.h"

#include "CamelCrusherVst2Effect.h"

#include "camelcrusher_recalled/LegacyParameters.h"
#include "camelcrusher_recalled/LegacySchema.h"

#import <AppKit/AppKit.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>

#ifndef CAMELCRUSHER_RECALLED_SOURCE_DIR
#define CAMELCRUSHER_RECALLED_SOURCE_DIR ""
#endif

namespace camelcrusher_recalled {
class CamelCrusherVst2EditorActionTarget {
 public:
  virtual ~CamelCrusherVst2EditorActionTarget() = default;
  virtual void previousProgramRequested() = 0;
  virtual void nextProgramRequested() = 0;
  virtual void programSelectionChanged(NSPopUpButton* popup) = 0;
  virtual void parameterControlChanged(NSControl* control) = 0;
  virtual void randomizeRequested() = 0;
};
}

@interface CamelCrusherKnobControl : NSControl
@end

@interface CamelCrusherSpriteToggleButton : NSButton
@property(nonatomic, strong) NSImage* spriteImage;
@property(nonatomic, assign) NSInteger frameCount;
@end

@interface CamelCrusherSpriteMomentaryButton : NSButton
@property(nonatomic, strong) NSImage* spriteImage;
@property(nonatomic, assign) NSInteger frameCount;
@property(nonatomic, assign) BOOL pressed;
@end

@interface CamelCrusherPatchArrowButton : NSButton
@property(nonatomic, assign) BOOL pointsLeft;
@end

@interface CamelCrusherVst2EditorPanel : NSView
@property(nonatomic, assign) camelcrusher_recalled::CamelCrusherVst2EditorActionTarget* owner;
- (NSRect)brandLinkRect;
- (void)handlePreviousProgram:(id)sender;
- (void)handleNextProgram:(id)sender;
- (void)handleProgramChange:(id)sender;
- (void)handleParameterChange:(id)sender;
- (void)handleRandomize:(id)sender;
@end

namespace {

constexpr CGFloat kOriginalWidth = 345.0;
constexpr CGFloat kOriginalHeight = 373.0;
constexpr VstInt16 kEditorWidth = 345;
constexpr VstInt16 kEditorHeight = 373;
constexpr CGFloat kBrandLinkX = 17.0;
constexpr CGFloat kBrandLinkY = 16.0;
constexpr CGFloat kBrandLinkWidth = 311.0;
constexpr CGFloat kBrandLinkHeight = 82.0;
constexpr CGFloat kPresetPopupFontSize = 14.0;
constexpr CGFloat kPresetMenuFontSize = 15.0;
constexpr CGFloat kRandomizeCenterX = 275.0;
constexpr CGFloat kRandomizeCenterY = 127.0;
constexpr CGFloat kRandomizeWidth = 93.0;
constexpr CGFloat kRandomizeHeight = 28.0;

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

NSColor* CamelCrusherBronzeDark() {
  return [NSColor colorWithCalibratedRed:0.22 green:0.16 blue:0.11 alpha:1.0];
}

NSColor* CamelCrusherBronzeMid() {
  return [NSColor colorWithCalibratedRed:0.47 green:0.33 blue:0.21 alpha:1.0];
}

NSColor* CamelCrusherBronzeHighlight() {
  return [NSColor colorWithCalibratedRed:0.79 green:0.61 blue:0.34 alpha:1.0];
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

  NSImage* cached = cache[resource_name];
  if (cached != nil) {
    return cached;
  }

  NSBundle* bundle = [NSBundle bundleForClass:[CamelCrusherVst2EditorPanel class]];
  NSString* path = [bundle pathForResource:[resource_name stringByDeletingPathExtension]
                                    ofType:[resource_name pathExtension]];
  if (path == nil) {
    path = camelCrusherSourceResourcePath(resource_name);
  }

  NSImage* image =
      path != nil ? [[NSImage alloc] initWithContentsOfFile:path] : nil;
  if (image != nil) {
    cache[resource_name] = image;
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

}  // namespace

@implementation CamelCrusherKnobControl {
  double dragOriginValue_;
  CGFloat dragOriginY_;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self != nil) {
    self.wantsLayer = YES;
  }
  return self;
}

- (BOOL)isOpaque {
  return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  NSImage* knob_sprite = camelCrusherResourceImage(@"Knob.png");
  if (knob_sprite != nil) {
    const NSInteger frame_count = 55;
    const CGFloat value =
        std::clamp(static_cast<CGFloat>(self.doubleValue), 0.0, 1.0);
    const NSInteger frame_index = static_cast<NSInteger>(
        std::llround(value * static_cast<CGFloat>(frame_count - 1)));
    drawSpriteFrame(knob_sprite, frame_index, frame_count, self.bounds);
    return;
  }

  NSRect bounds = NSInsetRect(self.bounds, 3.0, 3.0);
  const CGFloat size = MIN(bounds.size.width, bounds.size.height);
  const NSPoint center = NSMakePoint(NSMidX(bounds), NSMidY(bounds));
  const CGFloat radius = (size * 0.5) - 3.0;

  NSRect outer_rect = NSMakeRect(center.x - radius, center.y - radius, radius * 2.0,
                                 radius * 2.0);
  NSBezierPath* outer_path = [NSBezierPath bezierPathWithOvalInRect:outer_rect];
  NSGradient* outer_gradient = [[NSGradient alloc]
      initWithColors:@[
        [NSColor colorWithCalibratedWhite:0.07 alpha:0.94],
        [NSColor colorWithCalibratedWhite:0.17 alpha:0.94],
      ]];
  [outer_gradient drawInBezierPath:outer_path angle:270.0];

  NSRect knob_rect = NSInsetRect(outer_rect, 5.5, 5.5);
  NSBezierPath* knob_path = [NSBezierPath bezierPathWithOvalInRect:knob_rect];
  NSGradient* knob_gradient = [[NSGradient alloc]
      initWithColors:@[
        CamelCrusherBronzeHighlight(),
        CamelCrusherBronzeMid(),
        CamelCrusherBronzeDark(),
      ]];
  [knob_gradient drawInBezierPath:knob_path angle:255.0];

  [[NSColor colorWithCalibratedWhite:1.0 alpha:0.14] setStroke];
  [knob_path setLineWidth:1.0];
  [knob_path stroke];

  const CGFloat value = static_cast<CGFloat>(self.doubleValue);
  const CGFloat indicator_angle =
      (-140.0 + (value * 280.0)) * static_cast<CGFloat>(M_PI) / 180.0;
  NSPoint indicator_end =
      NSMakePoint(center.x + std::cos(indicator_angle) * (radius - 13.0),
                  center.y + std::sin(indicator_angle) * (radius - 13.0));
  [CamelCrusherProgramTextColor() setStroke];
  NSBezierPath* indicator = [NSBezierPath bezierPath];
  [indicator moveToPoint:center];
  [indicator lineToPoint:indicator_end];
  [indicator setLineWidth:3.0];
  [indicator setLineCapStyle:NSLineCapStyleRound];
  [indicator stroke];

  NSRect cap_rect = NSMakeRect(center.x - 7.5, center.y - 7.5, 15.0, 15.0);
  NSBezierPath* cap_path = [NSBezierPath bezierPathWithOvalInRect:cap_rect];
  NSGradient* cap_gradient = [[NSGradient alloc]
      initWithColors:@[
        [NSColor colorWithCalibratedWhite:0.42 alpha:0.95],
        [NSColor colorWithCalibratedWhite:0.18 alpha:0.95],
      ]];
  [cap_gradient drawInBezierPath:cap_path angle:270.0];
  [[NSColor colorWithCalibratedWhite:0.03 alpha:0.85] setStroke];
  [cap_path setLineWidth:1.0];
  [cap_path stroke];
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

@implementation CamelCrusherSpriteToggleButton

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
  drawSpriteFrame(self.spriteImage, frame_index, self.frameCount, self.bounds);
}

- (void)mouseDown:(NSEvent*)event {
  self.state = (self.state == NSControlStateValueOn) ? NSControlStateValueOff
                                                     : NSControlStateValueOn;
  [self setNeedsDisplay:YES];
  [self sendAction:self.action to:self.target];
}

@end

@implementation CamelCrusherSpriteMomentaryButton

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
  drawSpriteFrame(self.spriteImage, frame_index, self.frameCount, self.bounds);
}

- (void)mouseDown:(NSEvent*)event {
  self.pressed = YES;
  [self setNeedsDisplay:YES];
  [self sendAction:self.action to:self.target];
  self.pressed = NO;
  [self setNeedsDisplay:YES];
}

@end

@implementation CamelCrusherPatchArrowButton

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

@implementation CamelCrusherVst2EditorPanel

- (BOOL)isFlipped {
  return YES;
}

- (BOOL)isOpaque {
  return YES;
}

- (NSRect)brandLinkRect {
  const CGFloat scale =
      std::min(self.bounds.size.width / kOriginalWidth,
               self.bounds.size.height / kOriginalHeight);
  const CGFloat content_width = std::round(kOriginalWidth * scale);
  const CGFloat content_height = std::round(kOriginalHeight * scale);
  const CGFloat offset_x =
      std::round((self.bounds.size.width - content_width) * 0.5);
  const CGFloat offset_y =
      std::round((self.bounds.size.height - content_height) * 0.5);
  return NSMakeRect(std::round(offset_x + (kBrandLinkX * scale)),
                    std::round(offset_y + (kBrandLinkY * scale)),
                    std::round(kBrandLinkWidth * scale),
                    std::round(kBrandLinkHeight * scale));
}

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  const CGFloat scale =
      std::min(self.bounds.size.width / kOriginalWidth,
               self.bounds.size.height / kOriginalHeight);
  const CGFloat content_width = std::round(kOriginalWidth * scale);
  const CGFloat content_height = std::round(kOriginalHeight * scale);
  const NSRect content_rect =
      NSMakeRect(std::round((self.bounds.size.width - content_width) * 0.5),
                 std::round((self.bounds.size.height - content_height) * 0.5),
                 content_width, content_height);

  NSImage* background = camelCrusherResourceImage(@"Background.png");
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
    openCamelCrusherBrandURL();
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
namespace {

struct ToggleLayout {
  std::size_t legacy_index;
  CGFloat x;
  CGFloat y;
  CGFloat width;
  CGFloat height;
  const char* resource_name;
};

struct KnobLayout {
  std::size_t legacy_index;
  CGFloat x;
  CGFloat y;
};

constexpr std::array<ToggleLayout, 4> kModuleToggleLayouts{{
    {0, 39.0, 175.0, 32.0, 24.0, "OnButton.png"},
    {3, 199.0, 175.0, 32.0, 24.0, "OnButton.png"},
    {6, 39.0, 280.0, 32.0, 24.0, "OnButton.png"},
    {9, 199.0, 280.0, 32.0, 24.0, "OnButton.png"},
}};

constexpr ToggleLayout kPhatLayout{8, 123.0, 319.0, 22.0, 20.0,
                                   "PhatButton.png"};

constexpr std::array<KnobLayout, 7> kKnobLayouts{{
    {2, 63.0, 214.0},
    {1, 123.0, 214.0},
    {4, 222.0, 214.0},
    {5, 282.0, 214.0},
    {7, 63.0, 319.0},
    {11, 222.0, 319.0},
    {10, 282.0, 319.0},
}};

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

class CamelCrusherVst2EditorView final : public AEffEditor,
                                         public CamelCrusherVst2EditorActionTarget {
 public:
  explicit CamelCrusherVst2EditorView(CamelCrusherVst2Effect& effect)
      : AEffEditor(&effect), effect_(effect) {
    bounds_.top = 0;
    bounds_.left = 0;
    bounds_.bottom = kEditorHeight;
    bounds_.right = kEditorWidth;
  }

  bool getRect(ERect** rect) override {
    if (rect == nullptr) {
      return false;
    }
    *rect = &bounds_;
    return true;
  }

  bool open(void* ptr) override {
    auto* parent_view = (__bridge NSView*)ptr;
    if (parent_view == nil) {
      return false;
    }

    systemWindow = ptr;
    if (root_view_ == nil) {
      buildViewHierarchy();
    }

    root_view_.frame = NSMakeRect(0.0, 0.0, kEditorWidth, kEditorHeight);
    root_view_.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    if (root_view_.superview != parent_view) {
      [parent_view addSubview:root_view_];
    }
    syncFromEffect();
    return true;
  }

  void close() override {
    if (root_view_ != nil) {
      [root_view_ removeFromSuperview];
    }
    systemWindow = nullptr;
  }

  void idle() override {
    syncFromEffect();
  }

  void previousProgramRequested() override {
    VstInt32 current_program = effect_.getProgram();
    if (current_program <= 0) {
      current_program = kLegacyProgramCount;
    }
    effect_.setProgram(current_program - 1);
    syncFromEffect();
  }

  void nextProgramRequested() override {
    VstInt32 current_program = effect_.getProgram();
    current_program = (current_program + 1) % kLegacyProgramCount;
    effect_.setProgram(current_program);
    syncFromEffect();
  }

  void programSelectionChanged(NSPopUpButton* popup) override {
    if (popup == nil) {
      return;
    }

    effect_.setProgram(static_cast<VstInt32>(popup.indexOfSelectedItem));
    syncFromEffect();
  }

  void parameterControlChanged(NSControl* control) override {
    if (control == nil) {
      return;
    }

    float value = 0.0F;
    if ([control isKindOfClass:[CamelCrusherSpriteToggleButton class]]) {
      auto* toggle = static_cast<CamelCrusherSpriteToggleButton*>(control);
      value = toggle.state == NSControlStateValueOn ? 1.0F : 0.0F;
    } else {
      auto* knob = static_cast<CamelCrusherKnobControl*>(control);
      value = static_cast<float>(knob.doubleValue);
    }

    effect_.setParameterAutomated(static_cast<VstInt32>(control.tag), value);
    syncFromEffect();
  }

  void randomizeRequested() override {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> unit(0.0F, 1.0F);
    std::bernoulli_distribution toggle(0.5);

    auto applyRandomizedParameter = [&](const VstInt32 parameter_index) {
      float randomized = unit(rng);
      const auto parameter_id =
          legacyParameterIdFromLegacyIndex(static_cast<std::size_t>(parameter_index));
      if (parameter_id.has_value() &&
          isSwitchLikeParameter(parameter_id.value())) {
        randomized = toggle(rng) ? 1.0F : 0.0F;
      }
      effect_.setParameterAutomated(parameter_index, randomized);
    };

    for (const auto& layout : kModuleToggleLayouts) {
      applyRandomizedParameter(static_cast<VstInt32>(layout.legacy_index));
    }
    applyRandomizedParameter(static_cast<VstInt32>(kPhatLayout.legacy_index));
    for (const auto& layout : kKnobLayouts) {
      applyRandomizedParameter(static_cast<VstInt32>(layout.legacy_index));
    }

    syncFromEffect();
  }

 private:
  void buildViewHierarchy() {
    root_view_ = [[CamelCrusherVst2EditorPanel alloc]
        initWithFrame:NSMakeRect(0.0, 0.0, kEditorWidth, kEditorHeight)];
    root_view_.owner = this;

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
        [[CamelCrusherPatchArrowButton alloc] initWithFrame:NSZeroRect];
    previous_program_button_.pointsLeft = YES;
    previous_program_button_.target = root_view_;
    previous_program_button_.action = @selector(handlePreviousProgram:);
    [root_view_ addSubview:previous_program_button_];

    next_program_button_ =
        [[CamelCrusherPatchArrowButton alloc] initWithFrame:NSZeroRect];
    next_program_button_.pointsLeft = NO;
    next_program_button_.target = root_view_;
    next_program_button_.action = @selector(handleNextProgram:);
    [root_view_ addSubview:next_program_button_];

    randomize_button_ =
        [[CamelCrusherSpriteMomentaryButton alloc] initWithFrame:NSZeroRect];
    randomize_button_.spriteImage = camelCrusherResourceImage(@"ButtonRandom.png");
    randomize_button_.target = root_view_;
    randomize_button_.action = @selector(handleRandomize:);
    [root_view_ addSubview:randomize_button_];

    for (std::size_t index = 0; index < kModuleToggleLayouts.size(); ++index) {
      const auto& layout = kModuleToggleLayouts[index];
      auto* toggle = [[CamelCrusherSpriteToggleButton alloc] initWithFrame:NSZeroRect];
      toggle.tag = static_cast<NSInteger>(layout.legacy_index);
      toggle.spriteImage =
          camelCrusherResourceImage([NSString stringWithUTF8String:layout.resource_name]);
      toggle.target = root_view_;
      toggle.action = @selector(handleParameterChange:);
      module_toggle_controls_[index] = toggle;
      [root_view_ addSubview:toggle];
    }

    phat_button_ = [[CamelCrusherSpriteToggleButton alloc] initWithFrame:NSZeroRect];
    phat_button_.tag = static_cast<NSInteger>(kPhatLayout.legacy_index);
    phat_button_.spriteImage =
        camelCrusherResourceImage([NSString stringWithUTF8String:kPhatLayout.resource_name]);
    phat_button_.target = root_view_;
    phat_button_.action = @selector(handleParameterChange:);
    [root_view_ addSubview:phat_button_];

    for (std::size_t index = 0; index < kKnobLayouts.size(); ++index) {
      auto* knob = [[CamelCrusherKnobControl alloc] initWithFrame:NSZeroRect];
      knob.tag = static_cast<NSInteger>(kKnobLayouts[index].legacy_index);
      knob.target = root_view_;
      knob.action = @selector(handleParameterChange:);
      knob_controls_[index] = knob;
      [root_view_ addSubview:knob];
    }

    layoutViewHierarchy();
  }

  void layoutViewHierarchy() {
    const CGFloat root_width = static_cast<CGFloat>(bounds_.right - bounds_.left);
    const CGFloat root_height = static_cast<CGFloat>(bounds_.bottom - bounds_.top);
    const CGFloat scale =
        std::min(root_width / kOriginalWidth, root_height / kOriginalHeight);
    const CGFloat content_width = std::round(kOriginalWidth * scale);
    const CGFloat content_height = std::round(kOriginalHeight * scale);
    const CGFloat offset_x = std::round((root_width - content_width) * 0.5);
    const CGFloat offset_y = std::round((root_height - content_height) * 0.5);
    const CGFloat scale_x = scale;
    const CGFloat scale_y = scale;

    preset_selector_background_.frame =
        scaledRect(scale_x, scale_y, offset_x, offset_y, 23.0, 113.0, 196.0, 30.0);
    preset_popup_.frame =
        scaledRect(scale_x, scale_y, offset_x, offset_y, 53.0, 117.0, 118.0, 18.0);
    previous_program_button_.frame =
        scaledRect(scale_x, scale_y, offset_x, offset_y, 23.0, 113.0, 29.0, 30.0);
    next_program_button_.frame =
        scaledRect(scale_x, scale_y, offset_x, offset_y, 189.0, 113.0, 30.0, 30.0);
    randomize_button_.frame =
        scaledCenteredRect(scale_x, scale_y, offset_x, offset_y,
                           kRandomizeCenterX, kRandomizeCenterY, kRandomizeWidth,
                           kRandomizeHeight);

    for (std::size_t index = 0; index < kModuleToggleLayouts.size(); ++index) {
      const auto& layout = kModuleToggleLayouts[index];
      module_toggle_controls_[index].frame = scaledCenteredRect(
          scale_x, scale_y, offset_x, offset_y, layout.x, layout.y, layout.width,
          layout.height);
    }

    phat_button_.frame = scaledCenteredRect(scale_x, scale_y, offset_x, offset_y,
                                            kPhatLayout.x, kPhatLayout.y,
                                            kPhatLayout.width, kPhatLayout.height);

    for (std::size_t index = 0; index < kKnobLayouts.size(); ++index) {
      const auto& layout = kKnobLayouts[index];
      knob_controls_[index].frame = scaledCenteredRect(
          scale_x, scale_y, offset_x, offset_y, layout.x, layout.y, 48.0, 48.0);
    }
  }

  void refreshProgramMenu() {
    const NSInteger previous_index = preset_popup_.indexOfSelectedItem;
    [preset_popup_ removeAllItems];

    char buffer[kVstMaxProgNameLen]{};
    for (VstInt32 program = 0; program < kLegacyProgramCount; ++program) {
      std::memset(buffer, 0, sizeof(buffer));
      if (!effect_.getProgramNameIndexed(0, program, buffer)) {
        std::snprintf(buffer, sizeof(buffer), "Program %d", program + 1);
      }
      NSString* title = [NSString stringWithUTF8String:buffer];
      [preset_popup_ addItemWithTitle:title];
      NSMenuItem* item = preset_popup_.lastItem;
      item.attributedTitle = styledProgramTitle(title);
    }

    const auto current_program = effect_.getProgram();
    if (current_program >= 0 && current_program < kLegacyProgramCount) {
      [preset_popup_ selectItemAtIndex:current_program];
    } else if (previous_index >= 0) {
      [preset_popup_ selectItemAtIndex:previous_index];
    }
  }

  void syncFromEffect() {
    if (root_view_ == nil) {
      return;
    }

    refreshProgramMenu();

    for (std::size_t index = 0; index < kModuleToggleLayouts.size(); ++index) {
      const auto& layout = kModuleToggleLayouts[index];
      const float value = effect_.getParameter(static_cast<VstInt32>(layout.legacy_index));
      module_toggle_controls_[index].state =
          value >= 0.5F ? NSControlStateValueOn : NSControlStateValueOff;
      [module_toggle_controls_[index] setNeedsDisplay:YES];
    }

    const float phat_value =
        effect_.getParameter(static_cast<VstInt32>(kPhatLayout.legacy_index));
    phat_button_.state =
        phat_value >= 0.5F ? NSControlStateValueOn : NSControlStateValueOff;
    [phat_button_ setNeedsDisplay:YES];

    for (std::size_t index = 0; index < kKnobLayouts.size(); ++index) {
      const auto& layout = kKnobLayouts[index];
      knob_controls_[index].doubleValue =
          effect_.getParameter(static_cast<VstInt32>(layout.legacy_index));
      [knob_controls_[index] setNeedsDisplay:YES];
    }
  }

  CamelCrusherVst2Effect& effect_;
  ERect bounds_{};
  CamelCrusherVst2EditorPanel* root_view_ = nil;
  NSImageView* preset_selector_background_ = nil;
  NSPopUpButton* preset_popup_ = nil;
  CamelCrusherPatchArrowButton* previous_program_button_ = nil;
  CamelCrusherPatchArrowButton* next_program_button_ = nil;
  CamelCrusherSpriteMomentaryButton* randomize_button_ = nil;
  std::array<CamelCrusherSpriteToggleButton*, 4> module_toggle_controls_{};
  CamelCrusherSpriteToggleButton* phat_button_ = nil;
  std::array<CamelCrusherKnobControl*, 7> knob_controls_{};
};

AEffEditor* createCamelCrusherVst2Editor(CamelCrusherVst2Effect& effect) {
  return new CamelCrusherVst2EditorView(effect);
}

}  // namespace camelcrusher_recalled
