#include "CamelCrusherVst3IDs.h"
#include "public.sdk/source/vst/hosting/module.h"

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"

#import <AppKit/AppKit.h>

#include "TestSupport.h"
#include "KnobEditorChecks.h"

int main(int argc, char** argv) {
  @autoreleasepool {
    auto host = Steinberg::owned(new Steinberg::Vst::HostApplication());
    std::string error;
    auto module = VST3::Hosting::Module::create(argc == 2 ? argv[1] : CAMELCRUSHER_VST3_BUNDLE_PATH, error);
    CC_CHECK(module != nullptr);
    auto controller = module->getFactory().createInstance<Steinberg::Vst::IEditController>(
        VST3::UID::fromTUID(camelcrusher_recalled::vst3::kCamelCrusherVst3ControllerUID));
    CC_CHECK(controller != nullptr);
    CC_CHECK(controller->initialize(host.get()) == Steinberg::kResultOk);

    auto* view = controller->createView(Steinberg::Vst::ViewType::kEditor);
    CC_CHECK(view != nullptr);
    CC_CHECK(view->isPlatformTypeSupported(Steinberg::kPlatformTypeNSView) ==
           Steinberg::kResultTrue);

    auto* parent = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 900.0, 600.0)];
    CC_CHECK(view->attached((__bridge void*)parent, Steinberg::kPlatformTypeNSView) ==
           Steinberg::kResultOk);
    CC_CHECK(parent.subviews.count == 1);
    checkMixKnob(parent, [&] { return controller->getParamNormalized(10); });

    Steinberg::ViewRect rect{};
    CC_CHECK(view->getSize(&rect) == Steinberg::kResultTrue);
    CC_CHECK(rect.getWidth() > 0);
    CC_CHECK(rect.getHeight() > 0);

    CC_CHECK(view->removed() == Steinberg::kResultOk);
    CC_CHECK(parent.subviews.count == 0);

    view->release();
    controller->terminate();
  }
  return 0;
}
