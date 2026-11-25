#include "pluginterfaces/vst2.x/aeffectx.h"
#include <dlfcn.h>

#import <AppKit/AppKit.h>

#include "TestSupport.h"
#include "KnobEditorChecks.h"

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

int main(int argc, char** argv) {
  @autoreleasepool {
    void* module = dlopen(argc == 2 ? argv[1] : CAMELCRUSHER_VST2_BINARY, RTLD_NOW | RTLD_LOCAL);
    CC_CHECK(module != nullptr);
    auto entry = reinterpret_cast<AEffect* (*)(audioMasterCallback)>(dlsym(module, "VSTPluginMain"));
    CC_CHECK(entry != nullptr);
    auto* aeffect = entry(hostCallback);
    CC_CHECK(aeffect != nullptr);
    aeffect->dispatcher(aeffect, effOpen, 0, 0, nullptr, 0.0f);
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
    checkMixKnob(parent, [&] { return aeffect->getParameter(aeffect, 10); });

    aeffect->dispatcher(aeffect, effEditClose, 0, 0, nullptr, 0.0f);
    CC_CHECK(parent.subviews.count == 0);
    aeffect->dispatcher(aeffect, effClose, 0, 0, nullptr, 0.0f);
  }
  return 0;
}
