#pragma once

#import <AppKit/AppKit.h>
#include "TestSupport.h"
#include <cmath>

// Supply events directly to the editor's tracking loop without touching the
// user's mouse, event queue, or visible windows.
@interface CCTrackingTestWindow : NSWindow
@property(nonatomic, strong) NSMutableArray<NSEvent*>* trackingEvents;
@end

@implementation CCTrackingTestWindow
- (NSEvent*)nextEventMatchingMask:(NSEventMask)mask {
  CC_CHECK(self.trackingEvents.count > 0);
  NSEvent* event = self.trackingEvents.firstObject;
  [self.trackingEvents removeObjectAtIndex:0];
  return event;
}
@end

namespace {
NSControl* findMixKnob(NSView* view) {
  if ([view isKindOfClass:NSControl.class] && view.tag == 10) {
    return (NSControl*)view;
  }
  for (NSView* child in view.subviews) {
    if (NSControl* knob = findMixKnob(child)) return knob;
  }
  return nil;
}

// Locate the gold indicator against the gray knob, independent of Retina scale.
double pointerX(NSControl* knob) {
  [knob setNeedsDisplay:YES];
  NSBitmapImageRep* image = [knob bitmapImageRepForCachingDisplayInRect:knob.bounds];
  [knob cacheDisplayInRect:knob.bounds toBitmapImageRep:image];
  double sum = 0, count = 0;
  for (NSInteger y = 0; y < image.pixelsHigh; ++y) {
    for (NSInteger x = 0; x < image.pixelsWide; ++x) {
      NSColor* pixel = [[image colorAtX:x y:y] colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
      if (pixel.alphaComponent > 0.8 && pixel.redComponent - pixel.blueComponent > 0.18 &&
          pixel.greenComponent - pixel.blueComponent > 0.07) {
        sum += (x + 0.5) / image.pixelsWide;
        ++count;
      }
    }
  }
  CC_CHECK(count > 0);
  return sum / count;
}

template <class ReadMix>
void checkMixKnob(NSView* root, ReadMix readMix) {
  [NSApplication sharedApplication];
  auto* window = [[CCTrackingTestWindow alloc] initWithContentRect:root.bounds
      styleMask:NSWindowStyleMaskBorderless backing:NSBackingStoreBuffered defer:YES];
  window.contentView = root;
  [root layoutSubtreeIfNeeded];
  NSControl* knob = findMixKnob(root);
  CC_CHECK(knob != nil);
  const double original = readMix();
  auto event = [&](NSEventType type, CGFloat y) {
    return [NSEvent mouseEventWithType:type location:NSMakePoint(100, y)
                         modifierFlags:0 timestamp:0 windowNumber:window.windowNumber
                               context:nil eventNumber:1 clickCount:1 pressure:1];
  };
  auto drag = [&](CGFloat delta) {
    window.trackingEvents = [@[event(NSEventTypeLeftMouseDragged, 100 + delta),
                               event(NSEventTypeLeftMouseUp, 100 + delta)] mutableCopy];
    [knob mouseDown:event(NSEventTypeLeftMouseDown, 100)];
  };
  drag(-320);
  CC_CHECK(readMix() == 0.0 && pointerX(knob) < 0.42);
  drag(80);
  CC_CHECK(std::abs(readMix() - 0.5) < 1e-5 && std::abs(pointerX(knob) - 0.5) < 0.08);
  drag(320);
  CC_CHECK(readMix() == 1.0 && pointerX(knob) > 0.58);
  drag(-40);
  CC_CHECK(std::abs(readMix() - 0.75) < 1e-5);
  drag((original - 0.75) * 160);
  CC_CHECK(std::abs(readMix() - original) < 1e-5);
}
}  // namespace
