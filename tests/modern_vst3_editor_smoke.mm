#include "CamelCrusherVst3Controller.h"

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"

#import <AppKit/AppKit.h>

#include <cassert>

int main() {
  @autoreleasepool {
    auto host = Steinberg::owned(new Steinberg::Vst::HostApplication());
    auto* controller = new camelcrusher_recalled::vst3::CamelCrusherVst3Controller();
    assert(controller != nullptr);
    assert(controller->initialize(host.get()) == Steinberg::kResultOk);

    auto* view = controller->createView(Steinberg::Vst::ViewType::kEditor);
    assert(view != nullptr);
    assert(view->isPlatformTypeSupported(Steinberg::kPlatformTypeNSView) ==
           Steinberg::kResultTrue);

    auto* parent = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 900.0, 600.0)];
    assert(view->attached((__bridge void*)parent, Steinberg::kPlatformTypeNSView) ==
           Steinberg::kResultOk);
    assert(parent.subviews.count == 1);

    Steinberg::ViewRect rect{};
    assert(view->getSize(&rect) == Steinberg::kResultTrue);
    assert(rect.getWidth() > 0);
    assert(rect.getHeight() > 0);

    assert(view->removed() == Steinberg::kResultOk);
    assert(parent.subviews.count == 0);

    view->release();
    controller->terminate();
    controller->release();
  }
  return 0;
}
