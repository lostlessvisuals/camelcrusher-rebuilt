#include "CamelCrusherVst2Effect.h"

#import <AppKit/AppKit.h>

#include "TestSupport.h"

namespace {

VstIntPtr hostCallback(AEffect* /*effect*/,
                       VstInt32 opcode,
                       VstInt32 /*index*/,
                       VstIntPtr /*value*/,
                       void* /*ptr*/,
                       float /*opt*/) {
  if (opcode == audioMasterVersion) {
    return 2400;
  }
  return 0;
}

}  // namespace

int main() {
  @autoreleasepool {
    camelcrusher_recalled::CamelCrusherVst2Effect effect(hostCallback);
    auto* aeffect = effect.getAeffect();
    CC_CHECK(aeffect != nullptr);
    CC_CHECK((aeffect->flags & effFlagsHasEditor) != 0);

    ERect* rect = nullptr;
    CC_CHECK(aeffect->dispatcher(aeffect, effEditGetRect, 0, 0, &rect, 0.0f) == 1);
    CC_CHECK(rect != nullptr);
    CC_CHECK((rect->right - rect->left) > 0);
    CC_CHECK((rect->bottom - rect->top) > 0);

    auto* parent = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 900.0, 600.0)];
    CC_CHECK(aeffect->dispatcher(aeffect, effEditOpen, 0, 0, (__bridge void*)parent, 0.0f) ==
           1);
    CC_CHECK(parent.subviews.count == 1);

    aeffect->dispatcher(aeffect, effEditClose, 0, 0, nullptr, 0.0f);
    CC_CHECK(parent.subviews.count == 0);
  }
  return 0;
}
