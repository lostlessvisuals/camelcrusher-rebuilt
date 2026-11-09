#include "CamelCrusherVst3Controller.h"

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"

#import <AppKit/AppKit.h>

#include "TestSupport.h"

int main() {
  @autoreleasepool {
    auto host = Steinberg::owned(new Steinberg::Vst::HostApplication());
    auto* controller = new camelcrusher_recalled::vst3::CamelCrusherVst3Controller();
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

    Steinberg::ViewRect rect{};
    CC_CHECK(view->getSize(&rect) == Steinberg::kResultTrue);
    CC_CHECK(rect.getWidth() > 0);
    CC_CHECK(rect.getHeight() > 0);

    CC_CHECK(view->removed() == Steinberg::kResultOk);
    CC_CHECK(parent.subviews.count == 0);

    view->release();
    controller->terminate();
    controller->release();
  }
  return 0;
}
